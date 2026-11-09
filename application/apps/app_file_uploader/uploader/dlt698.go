package uploader

import (
	"encoding/binary"
	"log"
)

type APDU struct {
	service uint8
	choice  uint8
	PIID    uint8
	OAD     uint32
}
type DLT698 struct {
	head   uint8
	step   uint8
	len    uint16
	ctrl   uint8
	sa     uint8
	la     uint8
	addr   [16]uint8
	ca     uint8
	hcs    uint16
	apdu   [1024]uint8
	fcs    uint16
	tail   uint8
	packet []byte
}

func NewDLT698(packet []byte, address []byte, size int) *DLT698 {
	dlt698 := new(DLT698)
	dlt698.packet = packet
	if size < 16 {
		copy(dlt698.addr[:size], address[:size])
		dlt698.sa = uint8(size)
		dlt698.ctrl = 0x43
	}
	return dlt698
}

func (dlt698 *DLT698) SetAddress(address []byte, size int) {
	if size < 16 {
		copy(dlt698.addr[:size], address[:size])
		dlt698.sa = uint8(size)
		dlt698.ctrl = 0x43
	}
}

func (dlt698 *DLT698) Tx(state State, contents []byte, size int) []byte {
	var cs uint16
	var hcs uint16
	var fcs uint16
	var i uint16
	var wr uint16

	buf := make([]byte, 4096)
	buf[0] = 0x68
	buf[1] = 0x00
	buf[2] = 0x00
	buf[3] = dlt698.ctrl
	buf[4] = dlt698.sa
	buf[5] = dlt698.la
	copy(buf[6:6+(dlt698.sa&0x0f)], dlt698.addr[:(dlt698.sa&0x0f)])
	wr = uint16((dlt698.sa & 0x0f) + 6)
	buf[wr] = dlt698.ca
	wr += 1
	hcs = uint16(wr)
	buf[wr] = 0x00 //hcs
	wr += 1
	buf[wr] = 0x00 //hcs
	wr += 1
	if state == GetTransferInfo {
		buf[wr] = 0x05
		wr += 1
		buf[wr] = 0x01
		wr += 1
		buf[wr] = 0x00
		wr += 1
		binary.LittleEndian.PutUint32(buf[wr:], 0xF0010300)
		wr += 4
		buf[wr] = 0x00
		wr += 1
		fcs = uint16(wr)
		buf[wr] = 0x00
		wr += 1
		buf[wr] = 0x00
		wr += 1
		buf[wr] = 0x16
		wr += 1
		binary.LittleEndian.PutUint16(buf[1:], wr-2)

		cs = 0xffff
		for i = 1; i < hcs; i++ {
			cs = UpdateCrc16(cs, buf[i])
		}
		binary.LittleEndian.PutUint16(buf[hcs:], GetCrc(cs))

		cs = 0xffff
		for i = 1; i < fcs; i++ {
			cs = UpdateCrc16(cs, buf[i])
		}
		binary.LittleEndian.PutUint16(buf[fcs:], GetCrc(cs))
		//fmt.Println(buf[:wr])
		return buf[:wr]
	} else if state == GetFailedBlock { //读取位图信息
		buf[wr] = 0x05
		wr += 1
		buf[wr] = 0x01
		wr += 1
		buf[wr] = 0x00 //dlt698.PIID
		wr += 1
		binary.LittleEndian.PutUint32(buf[wr:], 0xF0010400)
		wr += 4
		buf[wr] = 0x00
		wr += 1
		fcs = uint16(wr)
		buf[wr] = 0x00
		wr += 1
		buf[wr] = 0x00
		wr += 1
		buf[wr] = 0x16
		wr += 1
		binary.LittleEndian.PutUint16(buf[1:], wr-2)

		cs = 0xffff
		for i = 1; i < hcs; i++ {
			cs = UpdateCrc16(cs, buf[i])
		}
		binary.LittleEndian.PutUint16(buf[hcs:], GetCrc(cs))

		cs = 0xffff
		for i = 1; i < fcs; i++ {
			cs = UpdateCrc16(cs, buf[i])
		}
		binary.LittleEndian.PutUint16(buf[fcs:], GetCrc(cs))
		return buf[:wr]
	} else if state == StartTransfer {
		buf[wr] = 0x07
		wr += 1
		buf[wr] = 0x01
		wr += 1
		buf[wr] = 0x00 //dlt698.PIID
		wr += 1
		binary.LittleEndian.PutUint32(buf[wr:], 0xF0010700)
		wr += 4
		copy(buf[wr:], contents[:size])
		wr += uint16(size)
		buf[wr] = 0x00
		wr += 1
		fcs = uint16(wr)
		buf[wr] = 0x00
		wr += 1
		buf[wr] = 0x00
		wr += 1
		buf[wr] = 0x16
		wr += 1
		binary.LittleEndian.PutUint16(buf[1:], wr-2)

		cs = 0xffff
		for i = 1; i < hcs; i++ {
			cs = UpdateCrc16(cs, buf[i])
		}
		binary.LittleEndian.PutUint16(buf[hcs:], GetCrc(cs))

		cs = 0xffff
		for i = 1; i < fcs; i++ {
			cs = UpdateCrc16(cs, buf[i])
		}
		binary.LittleEndian.PutUint16(buf[fcs:], GetCrc(cs))
		return buf[:wr]
	} else if state == Transfering {
		buf[wr] = 0x07 //0x05
		wr += 1
		buf[wr] = 0x01
		wr += 1
		buf[wr] = 0x00 //dlt698.PIID
		wr += 1

		//binary.LittleEndian.PutUint32(buf[wr:], 0xF0010400) F0010800
		binary.LittleEndian.PutUint32(buf[wr:], 0xF0010800)
		wr += 4
		copy(buf[wr:], contents[:size])
		wr += uint16(size)
		//data todo

		buf[wr] = 0x00
		wr += 1
		fcs = uint16(wr)
		buf[wr] = 0x00
		wr += 1
		buf[wr] = 0x00
		wr += 1
		buf[wr] = 0x16
		wr += 1
		binary.LittleEndian.PutUint16(buf[1:], wr-2)

		cs = 0xffff
		for i = 1; i < hcs; i++ {
			cs = UpdateCrc16(cs, buf[i])
		}
		binary.LittleEndian.PutUint16(buf[hcs:], GetCrc(cs))

		cs = 0xffff
		for i = 1; i < fcs; i++ {
			cs = UpdateCrc16(cs, buf[i])
		}
		binary.LittleEndian.PutUint16(buf[fcs:], GetCrc(cs))
		//fmt.Println(buf[:wr])
		return buf[:wr]
	}
	return nil
}

func (dlt698 *DLT698) OnRx(rb *RingBuffer, n int, rx chan Command) int {
	var i int
	var rd uint16
	var cs uint16
	var start uint16
	head := false

	for i = 0; i < rb.Length(); i++ {
		b, _ := rb.PeekByte()
		if b != 0x68 {
			rb.ReadByte()
		} else {
			head = true
			break
		}
	}
	n, _ = rb.PeekData(dlt698.packet)
	if head {
		//packet[0] == 0x68
		rd = 1
		start = rd
		dlt698.len = binary.LittleEndian.Uint16(dlt698.packet[rd:])
		dlt698.len &= 0x3fff
		if dlt698.len+2 > uint16(n) {
			return 0
		}
		rd += 2
		dlt698.ctrl = dlt698.packet[rd]
		rd += 1
		dlt698.sa = dlt698.packet[rd]
		rd += 1
		dlt698.la = dlt698.packet[rd]
		rd += 1
		copy(dlt698.addr[0:(dlt698.sa&0x0f)], dlt698.packet[rd:rd+uint16(dlt698.sa&0x0f)])
		rd += uint16(dlt698.sa & 0x0f)
		dlt698.ca = dlt698.packet[rd]
		rd += 1
		cs = 0xffff
		for i := start; i < rd; i++ {
			cs = UpdateCrc16(cs, dlt698.packet[i])
		}

		dlt698.hcs = binary.LittleEndian.Uint16(dlt698.packet[rd:])
		if GetCrc(cs) == dlt698.hcs {
			rd += 2
			apduSize := dlt698.len - 11 - uint16(dlt698.sa&0x0f) + 1
			copy(dlt698.apdu[:apduSize], dlt698.packet[rd:rd+apduSize])
			rd += apduSize

			cs = 0xffff
			for i := start; i < rd; i++ {
				cs = UpdateCrc16(cs, dlt698.packet[i])
			}
			dlt698.fcs = binary.LittleEndian.Uint16(dlt698.packet[rd:])
			if GetCrc(cs) == dlt698.fcs {
				rd += 2
				if dlt698.packet[rd] == 0x16 {
					log.Printf("<--%x\n", dlt698.packet[:n])
					log.Printf("apdu<--%x\n", dlt698.apdu[:apduSize])
					m := APDU{}
					m.service = dlt698.apdu[0]
					m.choice = dlt698.apdu[1]
					m.PIID = dlt698.apdu[2]
					m.OAD = binary.LittleEndian.Uint32(dlt698.apdu[3:])
					if m.OAD == 0xF0010300 {
						//log.Println("0xF0010300<--%x\n",dlt698.apdu[7:11])
						rx <- Command{GetTransferInfo, 0, dlt698.apdu[:apduSize]}
					} else if m.OAD == 0xF0010400 {
						//log.Println("0xF0010400<--%x\n",dlt698.apdu[7:11])
						rx <- Command{GetFailedBlock, 0, dlt698.apdu[:apduSize]}
					} else if m.OAD == 0xF0010700 {
						rx <- Command{StartTransfer, 0, dlt698.apdu[:apduSize]}
					} else if m.OAD == 0xF0010800 {
						rx <- Command{Transfering, 0, dlt698.apdu[:apduSize]}
					}
					for i := 0; i < int(rd); i++ {
						rb.ReadByte()
					}

					return 1
				}
			}
			for i := 0; i < int(rd); i++ {
				rb.ReadByte()
			}
		}
	}

	return 0
}
