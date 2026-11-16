package uploader

import (
	"context"
	"encoding/binary"
	"fmt"
	"log"
	"time"

	"github.com/albenik/go-serial"
)

type TransferSession struct {
	state        State
	Version      uint32 //版本信息
	FileSize     uint32 //文件大小 double-long-unsigned
	BlockSize    uint32 //128,256,512,1024,2048
	TransferMode uint32 //0-全包,1-定点,2-差分
	BlockNum     uint32
	BlockBits    []byte
	BlockBitsNum uint32
	Contents     []byte
	FileHash     uint16 //校验值
}

type Protocol interface {
	OnRx(rb *RingBuffer, n int, rx chan Command) int
	Tx(state State, contents []byte, size int) []byte
	SetAddress(address []byte, size int)
}

type State int

const (
	//stop orcancel
	Stop State = iota
	//暂停
	Suspend
	//恢复继续
	Resume
	TransferOver
	TransferFailed
	//请求设备地址
	AskDeviceAddress
	//获取文件传输状态
	GetTransferInfo
	//读取位图信息
	GetFailedBlock
	//启动传输状态
	StartTransfer
	//传输中
	Transfering
)

type Command struct {
	state State
	seq   int
	data  []byte
}

type Uploader struct {
	IsConnected    bool
	debug          bool
	Session        *TransferSession
	StreamTransfer bool
	tx             chan Command //handshake resumeBroken first next last restart
	rx             chan Command
	timer          *time.Timer
	seq            int
	Timeout        time.Duration
	try            int
	MaxTry         int
	progress       chan int
	ui             chan int
	port           serial.Port
	mode           serial.Mode
	Protocol       Protocol
	rb             *RingBuffer
	command        Command
}

func NewUploader(ctx context.Context, progress chan int, ui chan int, rx chan Command) *Uploader {
	tx := make(chan Command, 5)
	mode := serial.Mode{
		BaudRate: 115200,
	}
	rb := NewRingBuffer(4096)
	timer := time.NewTimer(time.Second)
	timer.Stop()
	return &Uploader{false, false, nil, false, tx, rx, timer, 0, 2, 0, 3,
		progress, ui, nil, mode, nil, rb,
		Command{GetTransferInfo, 0, nil},
	}
}

func Upload(ctx context.Context, uploader *Uploader) {
	go uploader.Run(ctx)
	go uploader.PortServe(ctx)
}

func (u *Uploader) SetSession(session *TransferSession) {
	u.Session = session
	u.seq = 0
	u.Session.state = GetTransferInfo
}

func (u *Uploader) SetDebug(debug bool) {
	u.debug = debug
}

func (u *Uploader) SetPort(portName string, mode serial.Mode) {
	if u.port != nil {
		log.Println("close port")
		u.port.Close()
	}
	port, err := serial.Open(portName, &mode)
	if err != nil {
		log.Println("XXXXXXXXXXXXXX")
		//log.Fatal(err)
	}
	u.port = port
	u.port.SetReadTimeout(100)
	//u.port.SetReadTimeoutEx(20000, 10000)
	//u.port.SetFirstByteReadTimeout(0xffffffff)
}

func (u *Uploader) SetPortMode(mode serial.Mode) {
	u.mode = mode
}

func (u *Uploader) SetProtocol(protocol Protocol) {
	u.Protocol = protocol
}

func (u *Uploader) GetTransferInfo() {
	u.tx <- Command{GetTransferInfo, 0, nil}
}

func (u *Uploader) PortServe(ctx context.Context) {
	var command Command
	var s string
	buff := make([]byte, 4096)
	buf := make([]byte, 1024)
	for {
		select {
		case <-ctx.Done():
			return
		//case <-u.timer.C:
		//u.rx <- 1
		case command = <-u.tx:
			//log.Println(u.Session.FileSize, u.Session.BlockSize, u.Session.BlockNum)
			//log.Printf("contents %x\n", u.Session.Contents[:128])
			if command.state == GetTransferInfo {
				buff = u.Protocol.Tx(command.state, nil, 0)
				s = "获取文件传输状态"
			} else if command.state == GetFailedBlock {
				buff = u.Protocol.Tx(command.state, nil, 0)
				s = "读取位图信息"
			} else if command.state == StartTransfer {
				binary.LittleEndian.PutUint32(buf, u.Session.Version)
				binary.LittleEndian.PutUint32(buf[4:], u.Session.FileSize)
				binary.LittleEndian.PutUint32(buf[8:], u.Session.BlockSize)
				binary.LittleEndian.PutUint32(buf[12:], u.Session.TransferMode)
				binary.LittleEndian.PutUint16(buf[16:], u.Session.FileHash)
				buff = u.Protocol.Tx(command.state, buf[:18], 18)
				s = "启动文件传输"
			} else if command.state == Transfering {
				s = fmt.Sprintf("传输中%d/%d", command.seq+1, u.Session.BlockNum)
				u.SetSeq(command.seq)
				start := command.seq * int(u.Session.BlockSize)
				end := command.seq * int(u.Session.BlockSize)
				blockSize := int(u.Session.BlockSize)
				if command.seq == int(u.Session.BlockNum)-1 {
					end = int(u.Session.FileSize)
					blockSize = int(u.Session.FileSize) - start
				}
				buff = u.Protocol.Tx(command.state, u.Session.Contents[start:end], blockSize)
			}
			log.Printf("%s-->%x\n", s, buff)
			//组帧并发送  地址自适应 波特率自适应 协议自适应
			//_, err := u.port.Write(u.contents[b*128 : (b+1)*128])
			_, err := u.port.Write(buff)
			if err != nil {
				//log.Fatal(err)
				//break
			}
			u.Session.state = command.state
			u.timer.Reset(time.Second)
		default:
			if u.Session.state >= AskDeviceAddress {
				n, err := u.port.Read(buf)
				if err != nil {
					//log.Fatal(err)
					//break
				}

				if n > 0 {
					u.rb.Write(buf[:n])
					u.Protocol.OnRx(u.rb, n, u.rx)
				}
			}
		}
	}
}

func (u *Uploader) GetNextSeq() int {
	for i := 0; i < int(u.Session.BlockNum); i++ {
		if u.Session.BlockBits[i/8]&(1<<(i&7)) == 0 {
			return i
		}
	}
	return -1
}

func (u *Uploader) SetSeq(i int) {
	u.Session.BlockBits[i/8] |= (1 << (i & 7))
}

func (u *Uploader) Run(ctx context.Context) {
	for {
		select {
		case command := <-u.rx:
			//case <-time.After(time.Second):
			u.timer.Stop()
			u.IsConnected = true
			if command.state == GetTransferInfo {
				u.command = Command{GetFailedBlock, 0, nil}
				u.tx <- u.command
			} else if command.state == GetFailedBlock {
				//更新BlockBits todo
				//断点续传 todo
				u.command = Command{StartTransfer, 0, nil}
				u.tx <- u.command
			} else if command.state == StartTransfer {
				u.command = Command{Transfering, 0, nil}
				u.tx <- u.command
			} else {
				u.seq = u.GetNextSeq()
				if u.seq == -1 {
					log.Println("传输完成")
					//todo 切换到断点续传
					u.ui <- 1
				} else {
					u.command = Command{Transfering, u.seq, nil}
					u.tx <- u.command
					u.progress <- u.seq
				}
			}

		case <-u.timer.C: //用来做超时处理和流式传输
			if u.debug == true {
				if u.Session.state == GetTransferInfo {
					u.command = Command{GetFailedBlock, 0, nil}
					u.tx <- u.command
				} else if u.Session.state == GetFailedBlock {
					u.command = Command{StartTransfer, 0, nil}
					u.tx <- u.command
				} else if u.Session.state == StartTransfer {
					u.command = Command{Transfering, 0, nil}
					u.tx <- u.command
				} else {
					u.seq = u.GetNextSeq()
					if u.seq == -1 {
						log.Println("传输完成")
						u.ui <- 1
					} else {
						u.command = Command{Transfering, u.seq, nil}
						u.tx <- u.command
						u.progress <- u.seq
					}
				}
			} else {
				if u.try > u.MaxTry {
					fmt.Println("超时重发次数已到")
					u.ui <- 0 //超时  界面通知 超时 传输失败
					u.timer.Stop()
				}
				fmt.Println("超时重发")
				u.tx <- u.command
				u.try += 1
			}
		case <-ctx.Done():
			u.timer.Stop()
			u.port.Close()
			if u.port != nil {
				fmt.Println("port closed")
			}
			return
		}
	}
}
