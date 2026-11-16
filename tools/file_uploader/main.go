package main

import (
	"context"
	"encoding/hex"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"strconv"
	"strings"
	. "uploader"

	"fyne.io/fyne/v2/cmd/fyne_settings/settings"

	"fyne.io/fyne/v2"

	"fyne.io/fyne/v2/app"
	"fyne.io/fyne/v2/container"
	"fyne.io/fyne/v2/dialog"
	"fyne.io/fyne/v2/layout"
	"fyne.io/fyne/v2/theme"
	"fyne.io/fyne/v2/widget"
	"github.com/albenik/go-serial"
	"github.com/flopp/go-findfont"
)

var Debug bool
var downloadButton *widget.Button
var fileSelection *widget.Button
var address *widget.Entry
var vision *widget.Entry
var progressBar *widget.ProgressBar
var w fyne.Window
var stop context.CancelFunc
var topWindow fyne.Window

//go build  -ldflags "-s -w -H windowsgui" main.go
func init() {
	fontPaths := findfont.List()
	for _, path := range fontPaths {
		// fmt.Println(path)
		//楷体:simkai.ttf
		//黑体:simhei.ttf
		if strings.Contains(path, "simhei.ttf") {
			os.Setenv("FYNE_FONT", path)
			break
		}
	}
}

func makeMenu(a fyne.App, w fyne.Window) *fyne.MainMenu {
	settingsItem := fyne.NewMenuItem("Settings", func() {
		w := a.NewWindow("Fyne Settings")
		w.SetContent(settings.NewSettings().LoadAppearanceScreen(w))
		w.Resize(fyne.NewSize(480, 480))
		w.Show()
	})

	// a quit item will be appended to our first (File) menu
	file := fyne.NewMenu("File", settingsItem)
	//if !fyne.CurrentDevice().IsMobile() {
	//file.Items = append(file.Items, fyne.NewMenuItemSeparator(), settingsItem)
	//}
	return fyne.NewMainMenu(
		file,
	)
}

func notify(ctx context.Context, ui chan int, progress chan int) {
	for {
		select {
		case <-ctx.Done():
			return
		case b := <-ui:
			if b == 1 {
				d := dialog.NewInformation("uploader", "传输完成", w)
				d.Show()
			} else {
				d := dialog.NewInformation("uploader", "传输失败", w)
				d.Show()
				//notification := fyne.NewNotification("hello", "World")
				//a.SendNotification(notification)
			}
			downloadButton.SetText("开始传输")
			stop()
		case p := <-progress:
			progressBar.SetValue((float64)(p))
		}
	}
}

func main() {
	defer func() {
		os.Unsetenv("FYNE_FONT")
	}()
	Debug = false
	a := app.NewWithID("file.transfer.v.1.0.1")
	a.SetIcon(theme.FyneLogo())
	//notification := fyne.NewNotification("hello", "World")
	//a.SendNotification(notification)
	w = a.NewWindow("File Uploader")
	topWindow = w

	ports, err := serial.GetPortsList()
	if err != nil {
		log.Fatal(err)
	}
	if len(ports) == 0 {
		log.Fatal("No serial ports found!")
	}
	for _, port := range ports {
		fmt.Printf("Found port: %v\n", port)
	}
	w.SetMainMenu(makeMenu(a, w))
	w.SetTitle("File Uploader v0.0.1")
	w.SetOnClosed(func() {
		a.Preferences().SetString("address", address.Text)
		a.Preferences().SetString("vision", vision.Text)
	})
	w.SetMaster()
	w.CenterOnScreen()
	//w.Resize(fyne.NewSize(400, 300))
	modeName := widget.NewLabel("升级方式")
	mode := widget.NewSelect([]string{"全包", "定点", "差分"}, func(s string) {
		a.Preferences().SetString("mode", s)
	})
	mode.SetSelected(a.Preferences().StringWithFallback("mode", "全包"))

	blockSizeLabel := widget.NewLabel("数据包大小")
	blockSize := widget.NewSelect([]string{"64", "192", "256", "512", "1024", "2048"}, func(s string) {
		a.Preferences().SetString("blockSize", s)
	})
	blockSize.SetSelected(a.Preferences().StringWithFallback("blockSize", "512"))
	sessionID := a.Preferences().IntWithFallback("sessionID", 1)

	protocolLabel := widget.NewLabel("传输协议选择")
	pro := widget.NewSelect([]string{"698", "DLMS", "645", "YModem"}, func(s string) {
		a.Preferences().SetString("protocol", s)
	})
	pro.SetSelected(a.Preferences().StringWithFallback("protocol", "698"))
	visionLabel := widget.NewLabel("版本信息")
	vision = widget.NewEntry()
	vision.SetText(a.Preferences().StringWithFallback("vision", "0000000000"))

	port_name := widget.NewLabel("串口")
	port2 := widget.NewSelect(ports, func(s string) {
		a.Preferences().SetString("port", s)
	})
	port2.SetSelected(a.Preferences().StringWithFallback("port", "select one"))
	parityLabel := widget.NewLabel("校验位")
	paritySelect := widget.NewSelect([]string{"无", "奇", "偶"}, func(s string) {
		a.Preferences().SetString("parity", s)
	})
	paritySelect.SetSelected(a.Preferences().StringWithFallback("parity", "无"))

	stopbitsLabel := widget.NewLabel("停止位")
	stopbitsSelect := widget.NewSelect([]string{"1", "1.5", "2"}, func(s string) {
		a.Preferences().SetString("stopbits", s)
	})
	stopbitsSelect.SetSelected(a.Preferences().StringWithFallback("stopbits", "1"))

	DataBitsLabel := widget.NewLabel("数据位")
	DataBits := widget.NewSelect([]string{"5", "6", "7", "8"}, func(s string) {
		a.Preferences().SetString("databits", s)
	})
	DataBits.SetSelected(a.Preferences().StringWithFallback("databits", "8"))
	BaudRateLabel := widget.NewLabel("波特率")
	BaudRate := widget.NewSelect([]string{"1200", "2400", "4800", "9600", "19200", "115200"}, func(s string) {
		a.Preferences().SetString("baudrate", s)
	})
	BaudRate.SetSelected(a.Preferences().StringWithFallback("baudrate", "9600"))

	fileLabel := widget.NewLabel("传输文件")
	fileSelection = widget.NewButtonWithIcon("Select File To Transfer", theme.FileIcon(), func() {
		fd := dialog.NewFileOpen(func(reader fyne.URIReadCloser, err error) {
			if err != nil {
				dialog.ShowError(err, w)
				return
			}
			if reader == nil {
				log.Println("Cancelled")
				return
			}
			a.Preferences().SetString("filename", reader.URI().Path())
			fileSelection.SetText(reader.URI().Path())
			//log.Printf("%x", uploader.contents[:128])
		}, w)
		//fd.SetFilter(storage.NewExtensionFileFilter([]string{".png", ".jpg", ".jpeg"}))
		fd.Show()
	})
	fileSelection.SetText(a.Preferences().StringWithFallback("filename", "Select File To Transfer"))

	addressLabel := widget.NewLabel("设备编号")
	address = widget.NewEntry()
	address.SetText(a.Preferences().StringWithFallback("address", "0000000000"))
	progressLabel := widget.NewLabel("传输进度")
	progressBar = widget.NewProgressBar()
	progressBar.Min = 0
	//transferLog := widget.NewMultiLineEntry()
	//cntScrolling := container.NewScroll(transferLog)
	//f := &logui{transferLog: transferLog, cntScrolling: cntScrolling}
	//log.SetOutput(f)
	downloadButton = widget.NewButton("开始传输", func() {
		if downloadButton.Text == "开始传输" {
			sessionID++
			//设置日志保存文件
			if Debug == false {
				ss := fmt.Sprintf("./%d.log", sessionID)
				a.Preferences().SetInt("sessionID", sessionID)
				f, _ := os.OpenFile(ss, os.O_WRONLY|os.O_CREATE|os.O_SYNC|os.O_APPEND, 0755)
				os.Stdout = f
				os.Stderr = f
				log.SetOutput(f)
			}
			ctx, cancel := context.WithCancel(context.Background())
			stop = cancel
			progres := make(chan int)
			ui := make(chan int)
			rx := make(chan Command)
			downloadButton.SetIcon(theme.DownloadIcon())
			downloadButton.SetText("停止传输")

			session := new(TransferSession)
			//准备传输文件
			fileName := a.Preferences().String("filename")
			if fileObj, err := os.Open(fileName); err == nil {
				defer fileObj.Close()
				if contents, err := ioutil.ReadAll(fileObj); err == nil {
					session.Contents = contents
					session.FileSize = uint32(len(contents))
					var cs uint16
					cs = 0xffff
					for i := 0; i < int(session.FileSize); i++ {
						cs = UpdateCrc16(cs, contents[i])
					}
					session.FileHash = GetCrc(cs)
				} else {
					return
				}
			}

			//准备传输协议
			packet := make([]byte, 4096)
			a.Preferences().SetString("address", address.Text)
			s := a.Preferences().StringWithFallback("address", "0000000000")
			if len(s)%2 != 0 {
				s = "0" + s
			}
			addr, _ := hex.DecodeString(s)
			protocol := NewDLT698(packet, addr, len(addr))

			//准备传输串口
			var parity serial.Parity
			var stopBits serial.StopBits
			s = a.Preferences().StringWithFallback("parity", "无")
			if s == "无" {
				parity = serial.NoParity
			} else if s == "奇" {
				parity = serial.OddParity
			} else {
				parity = serial.EvenParity
			}

			s = a.Preferences().StringWithFallback("baudrate", "9600")
			baudrate, _ := strconv.Atoi(s)

			s = a.Preferences().StringWithFallback("databits", "8")
			databits, _ := strconv.Atoi(s)

			s = a.Preferences().StringWithFallback("stopbits", "1")
			if s == "1" {
				stopBits = serial.OneStopBit
			} else if s == "1.5" {
				stopBits = serial.OnePointFiveStopBits
			} else {
				stopBits = serial.TwoStopBits
			}

			mode := serial.Mode{baudrate, databits, parity, stopBits}

			//准备传输控制方式
			s = a.Preferences().StringWithFallback("blockSize", "512")
			block_size, _ := strconv.Atoi(s)
			session.BlockSize = uint32(block_size)

			s = a.Preferences().StringWithFallback("mode", "全包")
			if s == "全包" {
				session.TransferMode = 0
			} else if s == "定点" {
				session.TransferMode = 1
			} else {
				session.TransferMode = 2
			}

			a.Preferences().SetString("vision", vision.Text)
			s = a.Preferences().StringWithFallback("vision", "0000000000")
			v, _ := strconv.Atoi(s)
			session.Version = uint32(v)

			session.BlockNum = (session.FileSize + session.BlockSize - 1) / session.BlockSize
			session.BlockBitsNum = (session.BlockNum + 7) / 8
			progressBar.Max = float64(session.BlockNum) - 1
			session.BlockBits = make([]byte, session.BlockBitsNum)
			s = a.Preferences().String("port")

			uploader := NewUploader(ctx, progres, ui, rx)
			uploader.SetDebug(Debug)
			uploader.SetPort(s, mode)
			uploader.SetProtocol(protocol)
			uploader.SetSession(session)
			go Upload(ctx, uploader)
			go notify(ctx, ui, progres)
			uploader.GetTransferInfo() //启动本次传输
		} else if downloadButton.Text == "暂停传输" {
			downloadButton.SetText("恢复传输")
		} else if downloadButton.Text == "恢复传输" {
			downloadButton.SetText("停止传输")
		} else if downloadButton.Text == "停止传输" {
			stop()
			downloadButton.SetText("开始传输")
			log.Println("停止传输")
		}
	})

	grid := container.New(layout.NewFormLayout(),
		fileLabel, fileSelection,
		modeName, mode,
		port_name, port2,
		BaudRateLabel, BaudRate,
		DataBitsLabel, DataBits,
		parityLabel, paritySelect,
		stopbitsLabel, stopbitsSelect,
		blockSizeLabel, blockSize,
		protocolLabel, pro,
		addressLabel, address,
		visionLabel, vision,
		progressLabel, progressBar,
	)
	//d := container.NewHSplit(container.New(layout.NewVBoxLayout(), grid, download), transferLog)
	w.SetContent(container.New(layout.NewVBoxLayout(), grid, downloadButton))

	w.ShowAndRun()
}
