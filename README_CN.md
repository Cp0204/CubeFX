# CubeFX

为你的 ZimaCube 注入多彩灯光！

CubeFX 是一个为 ZimaCube 设计的**第三方**开源灯光控制系统，基于 ESP32-C3 和 WS2812B 灯带。它可以让你自定义灯光效果、颜色和速度，并通过 Web 页面或按键进行控制。

[![GitHub release](https://img.shields.io/github/v/release/Cp0204/CubeFX.svg)](https://github.com/Cp0204/CubeFX/releases/latest) [![GitHub license](https://img.shields.io/github/license/Cp0204/CubeFX.svg)](https://github.com/Cp0204/CubeFX/blob/main/LICENSE)

[![Demo](https://img.youtube.com/vi/K5UVmzoG0bY/0.jpg)](https://www.youtube.com/watch?v=K5UVmzoG0bY)

## 特性

* [x] **多种灯光效果：** 内置多种灯光效果，并支持自定义效果
* [x] **控制界面：** 通过 Web 页面进行简单控制
* [x] **关闭热点：** 可关闭 WiFi 热点，按键或断电重新打开
* [x] **支持OTA更新：** 无需拆卸即可通过 WiFi 更新固件
* [x] **多种颜色格式：** 兼容 rgb, hsv, hex 三种颜色提交格式
* [x] **断电记忆：** 自动保存灯光设置到 EEPROM，断电不丢失
* [ ] ~~**兼容ZimaOS协议：** 可用 ZimaOS 控制灯光效果（等待官方公布文档）~~

## 使用方法

### 方式一：OTA更新

* 在 [Releases](https://github.com/Cp0204/CubeFX/releases/latest) 页面下载 `CubeFX_ota_xxx.bin` 固件
* 连接 `ZimaCube` 热点，默认密码 `homecloud`
* 浏览器访问 http://172.16.1.1
* 将 `.bin` 文件上传到 ESP32-C3 开发板

### 方式二：Web线刷

* 打开 [CubeFX installer](https://play.cuse.eu.org/cubefx)
* 使用 type-c 数据线连接 ESP32-C3
* 根据引导进行固件更新


### 自定义灯珠颜色

连接热点打开 http://172.16.1.1/post

或使用其它软件 POST 数据到 `http://172.16.1.1/post` ，数据格式如下：

```json
{
    "on": 1,
    "id": 5,
    "speed": 128,
    "lightness": 255,
    "data": [
        {
            "h": 0,
            "s": 100,
            "v": 100
        },
        {
            "r": 0,
            "g": 255,
            "b": 0
        },
        "0000FF"
    ]
}
```

* **on:** 灯光开关，可选，[0,1]
* **id:** 灯光效果，目前的有效范围是 [-71,5]
* **speed:** 效果速度，范围 [0,255]
* **lightness:** 亮度，范围 [0,255]
* **data:** 支持 RGB, HSV, HEX 三种颜色提交格式，其中 HEX 为 6 位十六进制颜色值。HSV 计划淘汰，请使用 RGB 或 HEX 格式。

## 捐赠

Enjoyed the project? Consider buying me a coffee - it helps me keep going!

<a href="https://buymeacoffee.com/cp0204"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" height="50" width="210" target="_blank"/></a>

如果你觉得这个项目对你有帮助，可以给我一点点支持，非常感谢～

![WeChatPay](https://cdn.jsdelivr.net/gh/Cp0204/Cp0204@main/img/wechat_pay_qrcode.png)

## Thank

- [kitesurfer1404/WS2812FX](https://github.com/kitesurfer1404/WS2812FX)
- [IceWhaleTech](https://github.com/IceWhaleTech)