# 韩国MOD-设备协议

版本：v1.1.3-alpha

更新于：2026-06-02

[TOC]

## 时间戳

时间戳是指格林尼治时间1970-01-01 00:00:00（北京时间1970-01-01 08:00:00）起至现在的总秒数。

如：北京时间2006-01-02 15:04:05的时间戳为1136185445，反之亦然。

## 协议规范

### MQTT协议规范

使用MQTT v3.1.1协议。

URL格式为：`mqtts://HOST:8883`

服务端的TCP keepalive周期为300秒。

#### CONNECT报文

* Will Flag：为0，不设置遗嘱标志。
* Clean Session：为1，清理会话。
* Keep Alive：180秒。
* Client Identifier：以“1v”开头，后接10位数字、大写、小写随机字符串，每次使用需重新生成。
* User Name：为设备序列号sn。
* Password：计算规则伪代码为：MD5(Client Identifier + User Name + salt).HEX().LOWER()，salt为“kx7#P3*2@vMq8z”。如Client Identifier为1v1234567890，User Name为11-22-33-44-55-66，则Password为91e96b960d0388fe8ae3fc1623c57f39。

#### CONNACK报文

* Connect Return code：当身份校验不通过时为5。可能的原因：
    * Client Identifier规则错误。
    * Client Identifier已被占用。
    * Password校验不通过。

#### PINGREQ报文

心跳不依赖上层应用的业务状态。

#### SUBSCRIBE报文

* Topic Filter：需订阅以下主题（其中{sn}需使用设备序列号代替）：
    * up/ack/{sn}
    * down/req/{sn}
* QoS：为0，至多一次。特殊情况另外说明。

#### PUBLISH报文

* QoS：为0，至多一次。特殊情况另外说明。
* RETAIN：为0，非保留消息。
* Topic Name：可发布以下主题（其中{sn}需使用设备序列号代替）：
    * up/req/{sn}
    * down/ack/{sn}
* Payload：最大1M。如过大服务端会把连接关闭。
    
    格式如下：

    ```
    VERSION
    SEQ [ACKSEQ]
    CMD [ACKCODE]
    [KEY VALUE]

    CONTENT
    ```

    以换行符\n分隔（不建议使用\r\n，虽然目前仍然支持），各行依次为：

    1. 格式版本号。目前为v1。
    1. 序号行。以空格分隔，各字段依次为：
        1. 包序号，字符串类型，建议为递增序列。
        1. Ack序号，字符串类型。与对应的请求包包序号一致。如非Ack包，则省略。
    1. 命令行。以空格分隔，各字段依次为：
        1. 命令名，字符串类型。Ack包的命令名与对应的请求包一致。
        1. Ack错误码，字符串类型。ok为成功，其它均为失败，枚举值见下文描述。如非Ack包，则省略。
    1. 首部行。可有0行至任意行。行内以空格分隔，各字段依次为：
        1. 首部键，字符串类型。目前未定义具体的枚举值。
        1. 首部值，字符串类型。
    1. 空行。
    1. 数据包内容，JSON字符串，需将空白符压缩。可省略。当JSON字符串没有字段时，也可把数据包内容省略。

    ACKCODE的枚举值如下：

    * invalid_installation_code：错误的设备安装码。
    * ok：成功。
    * sn_is_online：已有另一个连接使用此SN登录上线。

#### 主题

客户端/服务端发布/订阅的主题如下：

```
       up/req/{sn}        down/req/{sn}
      ------------->      ------------->
client              server              client
      <-------------      <-------------
       up/ack/{sn}        down/ack/{sn}
```

### WebSocket协议规范

服务端的TCP keepalive周期为300秒。

URL格式为：

* wss://HOST:14306/websocket：设备双向收发消息。

#### 心跳

设备发送WebSocket的ping message，服务端回复WebSocket的pong message。

心跳周期为120秒。设备/服务端在1.5个心跳周期内没有收到对端的WebSocket消息（ping/pong/text等message），则断开连接。

心跳不依赖上层应用的业务状态。

#### 数据包格式

数据包填充于WebSocket的text message，为JSON格式。数据包大小最大为1M。

请求格式如下：

```json
{
    "seq": 1,
    "sseq": 0,
    "cmd": "CMD",
    "FIELD": VALUE
}
```

| 参数  | 类型   | 说明           | 备注               |
|-------|--------|--------------|--------------------|
| seq   | uint64 | 客户端包序列号 | 客户端发送的包使用 |
| sseq  | uint64 | 服务端包序列号 | 服务端发送的包使用 |
| cmd   | string | 命令名         |                    |
| FIELD | any    | 命令定义的字段 | 可定义0个至任意个  |

下文不再详细描述请求seq、sseq、cmd字段的含义，只描述各协议自定义字段。

响应格式如下：

```json
{
    "seq": 1,
    "sseq": 0,
    "cmd": "CMDACK",
    "result": "RESULT",
    "reason": "REASON",
    "FIELD": VALUE
}
```

| 参数   | 类型   | 说明           | 备注                                           |
|--------|--------|--------------|----------------------------------------------|
| seq    | uint64 | 客户端包序列号 | 客户端接收的包使用。与请求的seq一致             |
| sseq   | uint64 | 服务端包序列号 | 服务端接收的包使用。与请求的sseq一致            |
| cmd    | string | 命令名         | 请求cmd的值拼接字符串Ack                       |
| result | string | 结果           | 成功为ok，失败为fail                            |
| reason | string | 失败原因       | 如result为ok，忽略此字段。具体值的含义见下文描述 |
| FIELD  | any    | 命令定义的字段 | 可定义0个至任意个                              |

下文不再详细描述响应seq、sseq、cmd、result、reason字段的含义，只描述各协议自定义字段。

设备需处理以下reason，但并非是所有的失败情况：

| reason               | 说明                     | 分类     |
|----------------------|------------------------|--------|
| sn is not registered | 设备序列号未在服务端登记 | 登录错误 |
| model mismatch       | 产品类目型号不匹配       | 登录错误 |
| sn is online         | 该设备序列号已在线       | 登录错误 |
| try again later      | 稍后重试                 | 同步错误 |
| device offline       | 设备不在线               | 设备错误 |

### 重连

设备连接失败或连接断开，则进行自动重连。自动重连时使用退避算法，随尝试的次数增加逐渐延长与下一次尝试间的时间间隔。计算方式伪代码如下（times为尝试的次数，首次为1）：

```c
int calculateReconnectSeconds(int times) {
    if (times > 9) {
        times = 9;
    }
    return 30 + (times -1) * 30 + random(0, 30);
}
```

由上可知，重连的时间间隔最短为30秒，最长为5分钟。退避算法执行最短22.5分钟、最长27分钟后，重连的时间间隔稳定在4.5分钟至5分钟之间。

设备也可于UI操作主动重连，此时触发的重连不需使用退避算法。

## 应用协议

设备需处理以下reason，但其并非是所有的失败情况：

| 参数                              | 说明                 | 分类        |
|-----------------------------------|---------------------|-------------|
| sn is not registered              | 设备序列号未登记      | 登录错误     |
| wrong password                    | 密码错误             | 登录错误     |
| unmatched network mode            | 不匹配的组网模式      | 登录错误     |
| sn is online                      | 该设备序列号已在线    | 登录错误     |
| this position has device          | 此位置已绑定设备      | 设备绑定错误 |
| device is in another position     | 设备已绑定至其它位置  | 设备绑定错误  |
| no license customer code          | 无授权系统客户码      | 授权码错误   |
| license exhausted                 | 授权码系统数量耗尽    | 授权码错误   |
| no person info                    | 无人员信息           | 人员信息错误  |
| no health info                    | 无健康码信息         | 健康码错误    |
| abnormal health info              | 健康码异常           | 健康码错误    |
| need to check health info         | 未校验健康码         | 健康码错误    |
| can not call manager              | 不能呼叫物管         | 呼叫错误      |
| room does not exist               | 呼叫的房号不存在      | 呼叫错误     |
| call nobody                       | 呼叫不到任何人        | 呼叫错误     |
| target is busy                    | 呼叫的目标占线忙碌    | 呼叫错误     |
| trtc is unavailable               | TRTC不可用           | 呼叫错误     |
| reject because of calling         | 正在呼叫拒绝监视      | 监视错误     |
| camera is being used              | 摄像头被占用          | 监视错误     |
| log does not exist                | 日志不存在            | 文件错误     |
| image does not exist              | 留影不存在            | 文件错误     |
| no lms license                    | 没有LMS人脸授权码     | 人脸注册错误  |
| device is upgrading               | 设备正在升级          | 人脸注册错误  |
| wrong card kind                   | 卡类型不正确          | 读卡错误      |
| sos notify nobody                 | SOS通知不到任何人     | SOS错误       |
| device offline                    | 设备不在线            | 通用错误      |

### 会话

#### 登录 —— login

更新于：v1.1.2

设备发送至服务器。

* 请求
	```
	{
		"sn": "11-22-33-AA-BB-CC",
		"model": "EL-S8-A0",
        "lowPower": true,
		"imei": "IMEI",
		"iccid": "ICCID",
		"networkMode": "wan",
		"lastSessionId": "7RYHlvv8jA04kp9YHGJ0",
		"kickOther": false
	}
	```

	| 参数          | 类型   | 说明                 | 备注                                             |
	|---------------|--------|----------------------|--------------------------------------------------|
	| sn            | string | 设备序列号           | 如使用MAC，格式为11-22-33-AA-BB-CC                |
	| model         | string | 产品类目型号         |                                                  |
	| lowPower      | bool   | 是否为低功耗设备     | 可省略                                           |
	| imei          | string | 设备SIM卡模块IMEI码  | 可省略。当设备使用SIM卡模块时使用                 |
	| iccid         | string | SIM卡ICCID           | 可省略。当设备使用SIM卡时使用                     |
	| networkMode   | string | 组网模式             | 可省略。局域网为lan，广域网为wan                   |
	| lastSessionId | string | 上一次登录的会话ID   | 可省略。用于设备掉线后快速登录                    |
	| kickOther     | bool   | 是否强行踢掉其它连接 | 可省略。慎用，使用不当会导致设备不断互相重复挤下线 |
* 响应
	```
	{
		"sessionId": "Y2OwebPDTX3dGehJXBX7"
	}
	```

	| 参数           | 类型   | 说明          | 备注                              |
	|----------------|--------|---------------|-----------------------------------|
	| sessionId      | string | 会话ID        |                                   |
	| ~~encryptKey~~ | string | 加密使用的KEY | base64编码，加密算法为AES-CBC      |
	| ~~encryptIv~~  | string | 加密使用的IV  | base64编码，加密算法为AES-CBC      |
	| ~~kind~~       | string | 设备类型      | 可省略。保安小叮为securityHandhold |

	如失败，需处理的reason：

	* sn is not registered
	* sn is online
	* unmatched network mode
	* wrong password
* 说明

	设备连接服务器后发送此协议，登录成功后才可发送其他协议。
	设备连接被断开，每隔10秒重新连接服务器并发送该协议。

#### 唤醒 —— wakeUp

服务器请求设备。

* 请求
    ```json
    {
    }
    ```

    | 参数       | 类型   | 说明   | 备注              |
    |------------|--------|--------|-------------------|
    | ~~event~~  | string | 事件   | 可省略。call为呼叫 |
    | ~~callId~~ | string | 呼叫ID |                   |
* 响应
* 说明

    低功耗设备使用。

#### 配网 —— provisioning

设备请求服务器。

* 请求
    ```json
    {
		"sn": "11-22-33-AA-BB-CC",
        "watchId": "WATCHID"
    }
    ```

    | 参数    | 类型   | 说明       | 备注 |
    |---------|--------|------------|------|
    | sn      | string | 设备序列号 |      |
    | watchId | string | 监听ID     |      |
* 响应
* 说明

    未使用login登录成功时，也可使用。

### 子设备

#### 协议代理 —— proxy

设备请求服务器、服务器请求设备。

* 请求
    ```json
    {
        "proxiedSn": "ABCD5678",
        "proxiedCmd": "CMD",
        "data": {
            "FIELD": VALUE
        }
    }
    ```

    | 参数       | 类型   | 说明               | 备注               |
    |------------|--------|------------------|--------------------|
    | proxiedSn  | string | 被代理子设备序列号 |                    |
    | proxiedCmd | string | 被代理子设备命令名 | 本文档定义的命令名 |
    | data       | object | 命令数据           | 可省略             |
    | data.FIELD | any    | 命令定义的字段     | 可定义0个至任意个  |
* 响应
    ```json
    {
        "proxiedSn": "ABCD5678",
        "proxiedCmd": "CMDACK",
        "data": {
            "FIELD": VALUE
        }
    }
    ```

    | 参数       | 类型   | 说明               | 备注                      |
    |------------|--------|------------------|---------------------------|
    | proxiedSn  | string | 被代理子设备序列号 |                           |
    | proxiedCmd | string | 被代理子设备命令名 | 请求proxiedCmd的值拼接Ack |
    | data       | object | 命令数据           | 可省略                    |
    | data.FIELD | any    | 命令定义的字段     | 可定义0个至任意个         |

#### 子设备上线 —— online

设备请求服务器。必需嵌入proxy协议。

* 请求
    ```json
    {
        "model": "EL-S8-A0"
    }
    ```

    | 参数  | 类型   | 说明         | 备注 |
    |-------|--------|------------|------|
    | model | string | 产品类目型号 |      |
* 响应

#### 子设备离线 —— offline

设备请求服务器。必需嵌入proxy协议。

* 请求
* 响应

### 升级

#### 升级 —— upgrade

设备请求服务器。

* 请求
    ```json
    {
        "scene": "remote",
        "model": "EL-S8-A0",
        "system": "android",
        "packaging": "systemPatch",
        "apk": "LH",
        "version": "1.2.0",
        "hardwareVersion": "3.4.5"
    }
    ```

    | 参数            | 类型   | 说明         | 备注                                                       |
    |-----------------|--------|--------------|------------------------------------------------------------|
    | scene           | string | 升级场景     | auto为设备自动升级，manual为设备手动升级，remote为远程升级   |
    | model           | string | 产品类目型号 |                                                            |
    | system          | string | 操作系统     | linux为Linux，android为安卓                                 |
    | packaging       | string | 打包方式     | 应用包为app，系统完整包为systemPack，系统差异包为systemPatch |
    | apk             | string | APK名        | 可省略。当system为android且packaging为app时使用             |
    | version         | string | 版本号       |                                                            |
    | hardwareVersion | string | 硬件版本号   |                                                            |
    | ~~name~~        | string | 名称         |                                                            |
	| ~~kind~~        | string | 类型         | 系统完整包为systemPack，系统差异包为systemPatch，安卓应用包为androidApp，Linux应用包为linuxApp |
	| ~~software~~    | string | 升级包名称   |                                                            |
* 响应
    ```json
    {
        "should": true,
        "model": "EL-S8-A0",
        "system": "android",
        "packaging": "systemPatch",
        "apk": "LH",
        "version": "1.2.3",
        "applyVersions": ["1.2.0"],
        "hardwareVersion": "3.4.5",
        "description": "1.修复若干bug。\n2.增加网络状态提示。",
        "md5": "FC3FF98E8C6A0D3087D515C0473F8677",
        "url": "https://domain.com/firmware/EL-S8-A0-v1.2.3_20060102150405_1234567.zip"
    }
    ```

    | 参数            | 类型         | 说明             | 备注                                                       |
    |-----------------|--------------|------------------|------------------------------------------------------------|
    | should          | bool         | 是否需要升级     | 如为false，则忽略后面的字段                                 |
    | model           | string       | 产品类目型号     |                                                            |
    | system          | string       | 操作系统         | linux为Linux，android为安卓                                 |
    | packaging       | string       | 打包方式         | 应用包为app，系统完整包为systemPack，系统差异包为systemPatch |
    | apk             | string       | APK名            | 可省略。当system为android且packaging为app时使用             |
    | version         | string       | 版本号           |                                                            |
    | applyVersions   | string array | 适用的版本号列表 | 可省略                                                     |
    | hardwareVersion | string       | 硬件版本号       |                                                            |
    | description     | string       | 描述             |                                                            |
    | md5             | string       | 文件MD5          | 大写16进制                                                 |
    | url             | string       | 文件下载URL      |                                                            |
    | ~~name~~        | string       | 名称             |                                                            |
	| ~~kind~~        | string       | 类型             | 系统完整包为systemPack，系统差异包为systemPatch，安卓应用包为androidApp，Linux应用包为linuxApp |
	| ~~applyVersion~~| string       | 适用的当前版本号     | 当kind为systemPatch时使用 |
	| ~~paths~~       | string array | 需升级的文件系统路径 | 可省略   |
* 说明

    每天03:00至04:00设备自动升级。

### 报告

#### 报告设备详情 —— reportDetail

设备请求服务器。

* 请求
	```json
	{
		"protocol_version": "2.0.0",
		"model": "EL-S8-A0",
		"system": "android",
		"application_version": "1.2.3",
		"system_version": "2.3.4",
		"hardware_version": "3.4.5",
		"apk": "HF",
		"call_modes_supported": ["callX", "voip"],
		"language": "zh-CN",
        "tfCard": "on",
        "tfCard_space_total": 1024,
        "tfCard_space_used": 256,
		"batteries": [{
			"kind": "dry",
			"power": 90
		}]
	}
	```

	| 参数                 | 类型         | 说明               | 备注                                                                  |
	|----------------------|--------------|--------------------|-----------------------------------------------------------------------|
	| protocol_version     | string       | 通信协议版本号     | 可省略。当前为2.0.0                                                    |
	| model                | string       | 产品类目型号       | 可省略                                                                |
	| system               | string       | 操作系统           | linux为Linux，android为安卓                                            |
	| application_version  | string       | 应用版本号         | 可省略                                                                |
	| system_version       | string       | 系统版本号         | 可省略                                                                |
	| hardware_version     | string       | 硬件版本号         | 可省略                                                                |
	| apk                  | string       | APK名              | 可省略                                                                |
	| call_modes_supported | string array | 支持的呼叫模式列表 | 优先级依次从高到低，可为callX、voip，参见“呼叫发起 —— callXInitiate”小节 |
	| language             | string       | 语言               | en为英语，ko为韩语，zh-CN为简体中文，zh-TW为繁体中文                     |
	| tfCard               | string       | TF卡状态           | on已接入，off为未接入                                                  |
	| tfCard_space_total   | int          | TF卡总容量         | 单位为字节                                                            |
	| tfCard_space_used    | int          | TF卡已用容量       | 单位为字节                                                            |
	| batteries            | object array | 电池列表           |                                                                       |
	| batteries.kind       | string       | 电池类型           | dry为干电池，li为锂电池                                                |
	| batteries.power      | int          | 电池电量           | 最低为0，最高为100                                                     |

	可报告部分字段，未报告的字段省略（亦不为null）。
* 响应

#### 报告设置 —— reportSetting

更新于：v1.1.2

设备发送至服务器。

* 请求
	```json
	{
		"password_open": "668899",

		"language": "zh-CN",
		"engineeringPassword": "666666",
		"uploadIllegalRecord": true,
		"uploadRecordImage": true,
		"icCardEncrypted": true,
		"faceRecognition": true,
		"faceAntiSpoofing": true,
		"faceRecognitionByPeopleAware": true,
		"oneFacePerCard": true,
		"faceRegistration": true,
		"syncOtherFaceEngine": true,
		"faceRecognitionThreshold": 10,
		"faceRecognitionTimeout": 10,
		"call": true,
		"callManager": true,
		"callPlayEarlyMediaTimeout": 10,
		"callRingTimeout": 10,
		"callConverseTimeout": 10,
		"hangUpCallAfterOpen": true,
		"selectCallee": true,
		"callOrder": "sequential",
		"visitorRemoteOpen": true,
		"passwordOpen": true,
		"qrcodeOpen": true,
		"openVoltage": "nc",
		"openDelay": 1,
		"doorMagneticDetection": true,
		"doorMagneticAlarmGracePeriod": 10,
		"doorMagneticVoltage": "nc",
		"tamperDetection": true,
		"qrcodeScanTimeout": 10,
		"uploadLog": true,
		"net": "wired",
		"microphoneVolume": 100,
		"speakerVolume": 100,
		"sipServerHost": "1.2.3.4",
		"sipServerPort": 5060,
		"sipUsername": "sip0",
		"sipPassword": "123456",
		"cameras": [{
			"sn": "FF-EE-DD-CC-BB-AA",
			"ip": "192.168.1.1",
			"account": "admin",
			"password": "pwd"
		}]
	}
	```

	| 参数                         | 类型          | 说明                  | 备注  |
	|------------------------------|--------------|-----------------------|-------|
	| password_open                | string       | 开锁密码               |   |
	| language                     | string       | 语言                  | 英语为en，韩语为ko，简体中文为zh-CN，繁体中文为zh-TW  |
	| engineeringPassword          | string       | 工程密码               |   |
	| uploadIllegalRecord          | bool         | 是否上传非法的记录      |   |
	| uploadRecordImage            | bool         | 是否自动上传留影        | 可省略，省略为true  |
	| icCardEncrypted              | bool         | 是否启用IC卡加密        |   |
	| faceRecognition              | bool         | 是否启用人脸识别        |   |
	| faceAntiSpoofing             | bool         | 是否启用人脸活体检测     |   |
	| faceRecognitionByPeopleAware | bool         | 是否启用人感触发人脸注册 |   |
	| oneFacePerCard               | bool         | 是否启用一卡一人脸      |   |
	| faceRegistration             | bool         | 是否启用人脸注册        |   |
	| syncOtherFaceEngine          | bool         | 是否同步其它引擎的人脸   |   |
	| faceRecognitionThreshold     | int          | 人脸识别阈值            |   |
	| faceRecognitionTimeout       | int          | 人脸识别超时时间        | 单位为秒  |
	| call                         | bool         | 是否启用呼叫            | 可省略，省略为true  |
	| callManager                  | bool         | 是否启用呼叫物管        |   |
	| callPlayEarlyMediaTimeout    | int          | 呼叫播放早期媒体超时     | 单位为秒  |
	| callRingTimeout              | int          | 呼叫振铃超时            | 单位为秒  |
	| callConverseTimeout          | int          | 呼叫通话超时            | 单位为秒  |
	| hangUpCallAfterOpen          | bool         | 开锁后是否挂断呼叫       |   |
	| selectCallee                 | bool         | 是否启用选择被叫方       |   |
	| callOrder                    | string       | 呼叫顺序                | 顺振为sequential，同振为simultaneous  |
	| visitorRemoteOpen            | bool         | 是否启用访客远程开门     | 可省略，省略为true  |
	| passwordOpen                 | bool         | 是否启用密码开门         |   |
	| qrcodeOpen                   | bool         | 是否启用二维码开门       |   |
	| openVoltage                  | string       | 开锁电平输出            | 常开为no，常闭为nc  |
	| openDelay                    | int          | 开锁延时                | 单位为秒  |
	| doorMagneticDetection        | bool         | 是否启用门磁检测         |   |
	| doorMagneticAlarmGracePeriod | int          | 门磁报警宽限期           | 单位为秒  |
	| doorMagneticVoltage          | string       | 门磁电平输出             | 常开为no，常闭为nc  |
	| tamperDetection              | bool         | 是否启用防拆检测         |   |
	| qrcodeScanTimeout            | int          | 二维码开门扫码超时       | 单位为秒  |
	| uploadLog                    | bool         | 是否自动上传日志         |   |
	| net                          | string       | 联网方式                | 4G为4g，4G路由为4gRouter，WiFi为wifi，有线为wired  |
	| microphoneVolume             | int          | 麦克风音量              |   |
	| speakerVolume                | int          | 扬声器音量              |   |
	| sipServerHost                | string       | SIP服务器地址           |   |
	| sipServerPort                | int          | SIP服务器端口           |   |
	| sipUsername                  | string       | SIP用户名               |   |
	| sipPassword                  | string       | SIP密码                 |   |
	| cameras                      | object array | 网络摄像机列表           |   |
	| cameras.sn                   | string       | 网络摄像机设备序列号      |   |
	| cameras.ip                   | string       | 网络摄像机IP             |   |
	| cameras.account              | string       | 网络摄像机账号           |   |
	| cameras.password             | string       | 网络摄像机密码           |   |
* 响应
* 说明

	本协议于设置变更时发送。

### 同步

#### 同步时间 —— syncTime

设备请求服务器。

* 请求
* 响应
	```json
	{
		"time": "20060102150405",
		"timeZone": "-07:00"
	}
	```

	| 参数     | 类型   | 说明           | 备注                        |
	|----------|--------|----------------|-----------------------------|
	| time     | string | 时间           | 格式为20060102150405        |
	| timeZone | string | 时间所在的时区 | 格式为-07:00，东八区为+08:00 |
* 说明

	设备开机后，发送此协议校准时间。

#### 同步数据 —— sync

设备请求服务器。

* 请求
	```json
	{
		"version": -1,
		"force": true
	}
	```

	| 参数    | 类型 | 说明       | 备注                                                         |
	|---------|------|------------|--------------------------------------------------------------|
	| version | int  | 数据版本号 | 初始化时发送-1                                               |
	| force   | bool | 是否强制同步 | 定时同步可省略，非定时同步时设为true。当为true时，即使响应的version和请求的version一致，也会将响应的refresh设为true |

* 响应
	```json
	{
		"refresh": true,
		"version": 2,
		"kind": "KIND",
		"lmsCode": "CODE",
		"addressCode": ["CODE1", "CODE2", "CODE3"],
		"location": ["", "1", "1", ""],
		"floor": "2",
		"liftControlStatus": "#",
		"liftControl": [{"l": "3", "p": 3, "s": "+"}],
		"management": {
			"cards": [{"n": "DDCCBBAA", "k": "34", "e": "20060102", "d": true}],
			"faces": [{"id": "ID", "e": "20060102", "d": true}]
		},
		"denyList": [{
			"f": "FID",
			"o": true,
		}],
		"zones": [{
			"id": "ID",
			"kind": "building",
			"addressCode": ["CODE1", "CODE2", "CODE3"],
			"location": ["", "1", "1"],
			"doors": [{
				"sn": "SN",
				"kind": "KIND",
				"name": "",
				"liftControl": [{"l": "3", "p": 3, "s": "+"}],
				"channelNames": ["1"]
			}],
			"rooms": [{
				"n": "203",
				"l": "2",
				"c": [{"n": "DDCCBBAA", "k": "34", "e": "20060102", "d": true}],
				"f": [{"id": "ID", "e": "20060102", "d": true}]
			}]
		}]
	}
	```

	| 参数                      | 类型         | 说明                 | 备注                                                                         |
	|---------------------------|--------------|----------------------|------------------------------------------------------------------------------|
	| refresh                   | bool         | 是否需要更新数据     | 如为false，则忽略后面的字段                                                   |
	| version                   | int          | 数据版本号           |                                                                              |
	| kind                      | string       | 设备类型             | AI摄像头为aiIpc，围墙机为wallDoor，单元机为unitDoor，车闸机为parkingDoor，保安分机为security，保安小叮为securityHandhold，五方对讲APP为fiveWayApp，五方对讲机为fiveWayDev，梯内梯控机为liftInside，梯外梯控机为liftOutside |
	| lmsCode                   | string       | 人脸服务器授权码     | 可为""                                                                       |
	| addressCode               | string array | 设备所在区域地址编码 |                                                                              |
	| location                  | string array | 设备位置             | 第1个元素为小区名，最后1个元素为门口机名。围墙机只有2个元素，单元机有3至5个元素 |
	| floor                     | string       | 设备楼层             | 可省略。只在kind为liftInside或liftOutside时使用                               |
	| liftControlStatus         | string       | 梯控状态             | 可省略。全开为+，全闭为-，可控为#                                               |
	| liftControl               | object array | 梯控表               | 可省略。只在kind为liftInside或liftOutside时使用                               |
	| liftControl.l             | string       | 梯控表楼层           | 只当liftControl.s为+、-、#时使用                                               |
	| liftControl.p             | int          | 提空表端口号         |                                                                              |
	| liftControl.s             | string       | 梯控表状态           | 常开为+，常闭为-，可控为#，开门按钮为o，关门按钮为c                              |
	| management                | object       | 物管信息             |                                                                              |
	| management.cards          | object array | 物管卡               |                                                                              |
	| management.cards.n        | string       | 物管卡物理卡号       | 大写或小写16进制，字符串顺序为高位字节至低位字节                              |
	| management.cards.k        | string       | 物管卡物理卡号类型   | 韦根26为26，韦根34为34，韦根66为66。如有不支持的类型，则忽略该management.cards   |
	| management.cards.e        | string       | 物管卡有效期         | 格式为20060102。可省略，缺省则为无限期                                         |
	| management.cards.d        | bool         | 物管卡是否禁用       | 可省略。缺省为未禁用                                                          |
	| management.faces          | object array | 物管人脸信息         |                                                                              |
	| management.faces.id       | string       | 物管人脸注册ID       |                                                                              |
	| management.faces.e        | string       | 物管人脸有效期       | 格式为20060102。可省略，缺省则为无限期                                         |
	| management.faces.d        | bool         | 物管人脸是否禁用     | 可省略。缺省为未禁用                                                          |
	| denyList                  | object array | 黑名单列表           |                                                                              |
	| denyList.f                | string       | 黑名单人脸ID         |                                                                              |
	| denyList.o                | bool         | 是否允许开锁         | 可省略。缺省为不允许                                                          |
	| zones                     | object array | 区域信息             |                                                                              |
	| zones.id                  | string       | 区域ID               |                                                                              |
	| zones.kind                | string       | 区域类型             | 小区为community，楼栋为building                                               |
	| zones.addressCode         | string array | 区域地址编码         |                                                                              |
	| zones.location            | string array | 区域位置             | 如zones.kind为building，有2至4个元素，第1个元素为小区名                        |
	| zones.doors               | object array | 区域的门口机         |                                                                              |
	| zones.doors.sn            | string       | 设备序列号           |                                                                              |
	| zones.doors.kind          | string       | 设备类型             | AI摄像头为aiIpc，围墙机为wallDoor，单元机为unitDoor，车闸机为parkingDoor，保安分机为security，保安小叮为securityHandhold，五方对讲APP为fiveWayApp，五方对讲机为fiveWayDev，梯内梯控机为liftInside，梯外梯控机为liftOutside |
	| zones.doors.name          | string       | 设备名               |                                                                              |
	| zones.doors.liftControl   | object array | 梯控                 | 可省略。只在zones.doors.kind为liftInside或liftOutside时使用                   |
	| zones.doors.liftControl.l | string       | 楼层                 | 只当zones.doors.liftControl.s为+、-、#时使用                                   |
	| zones.doors.liftControl.p | int          | 电梯端口号           |                                                                              |
	| zones.doors.liftControl.s | string       | 梯控状态             | 常开为+，常闭为-，可控为#，开门按钮为o，关门按钮为c                              |
	| zones.doors.channelNames  | string array | 通道名列表           | 可省略                                                                       |
	| zones.rooms               | object array | 房间信息             |                                                                              |
	| zones.rooms.n             | string       | 房号                 | 可有特殊的房号""                                                             |
	| zones.rooms.l             | string       | 楼层                 |                                                                              |
	| zones.rooms.c             | object array | 住户卡               | 可省略                                                                       |
	| zones.rooms.c.n           | string       | 住户卡物理卡号       | 大写或小写16进制，字符串顺序为高位字节至低位字节                              |
	| zones.rooms.c.k           | string       | 住户卡物理卡号类型   | 韦根26为26，韦根34为34，韦根66为66。如有不支持的类型，则忽略该management.cards   |
	| zones.rooms.c.e           | string       | 住户卡有效期         | 格式为20060102。可省略，缺省则为无限期                                         |
	| zones.rooms.c.d           | bool         | 住户卡是否禁用       | 可省略。缺省为未禁用                                                          |
	| zones.rooms.f             | object array | 住户人脸             | 可省略                                                                       |
	| zones.rooms.f.id          | string       | 住户人脸注册ID       |                                                                              |
	| zones.rooms.f.e           | string       | 住户人脸有效期       | 格式为20060102。可省略，缺省则为无限期                                         |
	| zones.rooms.f.d           | bool         | 住户人脸是否禁用     | 可省略。缺省为未禁用                                                          |
* 说明

	默认15分钟一次。

#### 同步密码 —— syncPassword

设备发送至服务器。

* 请求
* 响应
	```json
	{
		"passwords": [{
			"p": "123456"
		}]
	}
	```

	| 参数        | 类型         | 说明     | 备注 |
	|-------------|--------------|----------|------|
	| passwords   | object array | 密码列表 |      |
	| passwords.p | string       | 密码     |      |

#### 获取报警提示音 —— getAlarmTone

设备发送至服务器。

* 请求
	```json
	{
		"tone": "alarm"
	}
	```

	| 参数 | 类型   | 说明       | 备注 |
	|------|--------|------------|------|
	| tone | string | 报警提示音 |      |
* 响应
	```json
	{
		"url": "https://domain.com/alarm.mp3"
	}
	```

	| 参数 | 类型   | 说明    | 备注 |
	|------|--------|---------|------|
	| url  | string | 下载URL |      |

#### 同步设置 —— syncSetting

更新于：v1.1.3

设备发送至服务器。

* 请求
* 响应
	```
	{
		"language": "zh-CN",
        "microphone_volume": 100,
		"speaker_volume_alarm": 100,
        "password_open": "668899",
        "doorContact_enabled": true,
        "doorContact_gracePeriod": 5,
        "battery_preferred": "dry",
        "record_storage_kind": "image",
        "smartHome_mode": "home",
		"alarm_tone": "alarm",
        "alarm_loitering_enabled": true,
        "alarm_loitering_gracePeriod": 30,
		"mask_enabled": true,
		"masks": [[[0.1, 0.1], [0.2, 0.1], [0.2, 0.2], [0.1, 0.2]]],
      
        "engineeringPassword": "666666",
		"uploadIllegalRecord": true,
		"uploadOpenImage": true,
		"icCardEncrypted": true,
		"enableCall": true,
		"callPlayEarlyMediaTimeout": 10,
		"callRingTimeout": 10,
		"callConverseTimeout": 10,
		"intervalBetweenEachCall": 10,
		"frequentlyCallPeriod": 600,
		"frequentlyCallTimes": 10,
		"callOrder": "sequential",
		"outgoingCallSips": ["sip2"],
		"outgoingCallRescueTels": ["15915915911"],
		"channelNames": ["通道1"],
		"channelNarrations": [{
			"channel": 1,
			"audio": "AUDIO",
			"url": "https://domain.com/a/b/c.mp3"
		}],
		"sipServerHost": "1.2.3.4",
		"sipServerPort": 5060,
		"sipUsername": "sip0",
		"sipPassword": "123456",
		"sipAccounts": [{
			"serverHost": "1.2.3.4",
			"serverPort": 5060,
			"username": "sip1",
			"password": "123456"
		}],
		"enableVisitorOpen": true,
		"openDelay": 1,
		"repairPressDuration": 5,
		"checkHealth": true,
		"checkHealthProvince": "GX",
		"microphoneVolume": 100,
		"speakerVolume": 100,
		"openCheckFaceAndCard": true
	}
	```

	| 参数                        | 类型                      | 说明                           | 备注                                              |
	|-----------------------------|---------------------------|--------------------------------|---------------------------------------------------|
	| language                    | string                    | 语言                           | en为英语，ko为韩语，zh-CN为简体中文，zh-TW为繁体中文 |
	| microphone_volume           | int                       | 麦克风音量                     | 最低为0，最高为100                                 |
	| speaker_volume_alarm        | int                       | 扬声器报警音量                 | 可省略。最弱为0，最强为100                          |
	| password_open               | string                    | 开锁密码                       |                                                   |
	| doorContact_enabled         | bool                      | 门磁是否启用                   |                                                   |
	| doorContact_gracePeriod     | int                       | 门磁报警宽限期                 | 单位为秒                                          |
	| battery_preferred           | string                    | 首选电池类型                   | dry为干电池，li为锂电池                            |
	| record_storage_kind         | string                    | 记录存储方式                   | image为图片，video为视频                           |
	| smartHome_mode              | string                    | 智能家居模式                   | away为离家模式，home为在家模式                     |
	| alarm_tone                  | string                    | 报警提示音                     |                                                   |
	| alarm_loitering_enabled     | bool                      | 报警长时间逗留是否启用         |                                                   |
	| alarm_loitering_gracePeriod | int                       | 报警长时间逗留宽限期           | 单位为秒                                          |
	| mask_enabled                | bool                      | 是否启用遮罩                   |                                                   |
	| masks                       | float64 array array array | 遮罩列表                       | 每个元素表示一个多边形；多边形每个元素表示一个顶点；每个顶点包含两个元素，分别是X轴和Y轴的相对位置，最小为0，最大为1 |
	| engineeringPassword         | string                    | 工程密码                       |                                                   |
	| uploadIllegalRecord         | bool                      | 是否上传非法的记录             |                                                   |
	| uploadOpenImage             | bool                      | 是否自动上传留影               | 可省略，省略为true                                 |
	| icCardEncrypted             | bool                      | 是否启用IC卡加密               |                                                   |
	| enableCall                  | bool                      | 是否启用呼叫                   | 可省略，省略为true                                 |
	| callPlayEarlyMediaTimeout   | int                       | 呼叫播放早期媒体超时           | 单位为秒                                          |
	| callRingTimeout             | int                       | 呼叫振铃超时                   | 单位为秒                                          |
	| callConverseTimeout         | int                       | 呼叫通话超时                   | 单位为秒                                          |
	| intervalBetweenEachCall     | int                       | 每次呼叫间的最少时间间隔       | 单位为秒                                          |
	| frequentlyCallPeriod        | int                       | 高频呼叫周期                   | 单位为秒                                          |
	| frequentlyCallTimes         | int                       | 高频呼叫次数限制               |                                                   |
	| callOrder                   | string                    | 呼叫顺序                       | 顺振为sequential，同振为simultaneous               |
	| outgoingCallSips            | string array              | 外呼SIP帐号列表                |                                                   |
	| outgoingCallRescueTels      | string array              | 外呼救援电话列表               |                                                   |
	| channelNames                | string array              | 通道名列表                     |                                                   |
	| channelNarrations           | object array              | 通道语音播报列表               |                                                   |
	| channelNarrations.channel   | int                       | 通道语音播报的通道             |                                                   |
	| channelNarrations.audio     | string                    | 通道语音播报的音频标识         |                                                   |
	| channelNarrations.url       | string                    | 通道语音播报的音频文件URL      |                                                   |
	| sipServerHost               | string                    | SIP服务器地址                  |                                                   |
	| sipServerPort               | int                       | SIP服务器端口                  |                                                   |
	| sipUsername                 | string                    | SIP用户名                      |                                                   |
	| sipPassword                 | string                    | SIP密码                        |                                                   |
	| sipAccounts                 | object array              | 共用的SIP帐号列表              |                                                   |
	| sipAccounts.serverHost      | string                    | 共用的SIP服务器地址            |                                                   |
	| sipAccounts.serverPort      | int                       | 共用的SIP服务器端口            |                                                   |
	| sipAccounts.username        | string                    | 共用的SIP用户名                |                                                   |
	| sipAccounts.password        | string                    | 共用的SIP密码                  |                                                   |
	| enableVisitorOpen           | bool                      | 是否启用访客远程开门           | 可省略，省略为true                                 |
	| openDelay                   | int                       | 开锁延时                       | 单位为秒                                          |
	| repairPressDuration         | int                       | 报修长按时长                   | 单位为秒                                          |
	| checkHealth                 | bool                      | 是否校验健康码                 |                                                   |
	| checkHealthProvince         | string                    | 校验健康码的省份               | 广西为GX                                          |
	| microphoneVolume            | int                       | 麦克风音量                     |                                                   |
	| speakerVolume               | int                       | 扬声器音量                     |                                                   |
	| openCheckFaceAndCard        | bool                      | 开锁是否需人脸和卡同时校验通过 |                                                   |
* 说明

	设备需过滤非法的设置值，若有更改，需发送“报告设置”协议。

#### 获取位置

更新于：v1.1.2

设备发送至服务器。

* 请求
	```
	{
		"seq": 1,
		"cmd": "getPosition"
	}
	```
* 响应
	```
	{
		"seq": 1,
		"cmd": "getPositionAck",
		"result": "ok"
		"community": "美丽花园",
		"communityId": "COMMUNITYID",
		"building": "1",
		"unit": "1",
		"floor": "1",
		"room": "101",
		"name": "前门"
	}
	```

	| 参数              | 类型          | 说明                  | 备注  |
	|-------------------|--------------|-----------------------|-------|
	| community         | string       | 小区名                 |   |
	| communityId       | string       | 小区ID                 |   |
	| building          | string       | 楼栋名                 | 不含“栋”字样  |
	| unit              | string       | 单元名                 | 不含“单元”字样  |
	| floor             | string       | 楼层                   |   |
	| room              | string       | 房号                   |   |
	| name              | string       | 设备名                 |   |

#### 获取人脸令牌 —— getFaceToken

设备请求服务器。

* 请求
	```json
	{
		"faceIds": ["FACEID"],
		"echoFaceIds": false
	}
	```

	| 参数        | 类型         | 说明               | 备注        |
	|-------------|--------------|--------------------|-------------|
	| faceIds     | string array | 人脸ID列表         | 最多50000个 |
	| echoFaceIds | bool         | 是否回显人脸ID列表 |             |
* 响应
	```json
	{
		"token": "TOKEN",
		"validSeconds": 600,
		"faceIds": ["FACEID"]
	}
	```

	| 参数         | 类型         | 说明               | 备注                                   |
	|--------------|--------------|--------------------|----------------------------------------|
	| token        | string       | 备份系统使用的令牌 |                                        |
	| validSeconds | int          | 有效期秒数         |                                        |
	| faceIds      | string array | 人脸ID列表         | 可省略。当请求的echoFaceIds为true时使用 |

#### 获取天气 —— getWeather

设备请求服务器。

* 请求
* 响应
	```json
    {
        "overview" "clear",
        "code": 100,
        "temperature": 25,
        "temperatureMin": 20,
        "temperatureMax": 30
    }
	```

    | 参数           | 类型   | 说明         | 备注                                                                           |
    |----------------|--------|--------------|--------------------------------------------------------------------------------|
    | overview       | string | 天气概况     | 可省略。clear为晴，cloudy为阴，dusty为沙尘，foggy为雾，hazy为霾，rainy为雨，snowy为雪 |
    | code           | int    | 天气代码     | 可省略。见下文描述                                                              |
    | temperature    | int    | 温度         | 可省略                                                                         |
    | temperatureMin | int    | 当天最低温度 | 可省略                                                                         |
    | temperatureMax | int    | 当天最高温度 | 可省略                                                                         |

    code具体值如下：

    * 1xx：对应overview为clear。
        * 100：晴。
    * 2xx：对应overview为cloudy。
        * 201：少云。
        * 202：多云。
        * 203：阴。
    * 3xx：对应overview为rainy。
        * 301：小雨。
        * 302：中雨。
        * 303：大雨。
        * 304：暴雨。
        * 310：阵雨。
        * 320：雷雨。
        * 332: 冻雨。
    * 4xx：对应overview为snowy。
        * 401：小雪。
        * 402：中雪。
        * 403：大雪。
        * 404：暴雪。
        * 410：阵雪。
        * 420：雨雪。
    * 5xx：对应overview为foggy。
        * 501：薄雾。
        * 502：大雾。
        * 503: 浓雾。
    * 6xx：对应overview为hazy。
        * 601: 轻度霾。
        * 602：中度霾。
        * 603：重度霾。
    * 7xx：对应overview为dusty。
        * 701：浮尘。
        * 702：沙尘暴。
* 说明

    返回次日天气。

#### 获取房间用户

更新于：v1.0.8

设备发送至服务器。

* 请求
	```
	{
		"seq": 1,
		"cmd": "getUsers",
		"scene": "call",
		"buildingId": "BUILDINGID",
		"target": ["101"]
	}
	```

	| 参数       | 类型         | 说明             | 备注                     |
	|------------|--------------|----------------|--------------------------|
	| scene      | string       | 场景             | 可省略。呼叫为call        |
	| buildingId | string       | 楼栋ID           | 只在围墙机呼叫住户时有用 |
	| target     | string array | 呼叫目标按键序列 | 对围墙机，buildingId为空则有2至3个元素，依次为栋号、单元号、房号（栋号、单元号至少有一）；buildingId非空则有1个元素，为房号。对单元机，则有1个元素，为房号。室内设备忽略此字段 |
* 响应
	```
	{
		"seq": 1,
		"cmd": "getUsersAck",
		"users": [{
			"userId": "USERID",
			"name": "张三",
			"tel": "15915915911",
			"image": "URL"
		}]
	}
	```

	| 参数         | 类型         | 说明     | 备注 |
	|--------------|--------------|--------|------|
	| users        | object array | 用户列表 |      |
	| users.userId | string       | 用户ID   |      |
	| users.name   | string       | 姓名     |      |
	| users.tel    | string       | 手机号   |      |
	| users.image  | string       | 头像URL  |      |

	如失败，需处理的reason：
	* room does not exist

### 五方通话

#### 获取小区可外呼列表

更新于：v1.1.1

设备发送至服务器。

* 请求
	```
	{
		"seq": 1,
		"cmd": "getOutgoingCallDevices",
		"communityId": "COMMUNITYID"
	}
	```

	| 参数                     | 类型          | 说明                   | 备注 |
	|--------------------------|--------------|------------------------|------|
	| communityId              | string       | 小区ID                  |   |
* 响应
	```
	{
		"seq": 1,
		"cmd": "getOutgoingCallDevicesAck",
		"result": "ok",
		"devices": [{
			"sn": "AA-BB-CC-11-22-33",
			"kind": "securityDev",
			"name": "值班室"
		}],
		"wechats": [{
			"userId": "USERID",
			"name": "张三"
		}]
	}
	```

	| 参数                  | 类型          | 说明                   | 备注 |
	|-----------------------|--------------|------------------------|------|
	| devices               | object array | 设备列表                |   |
	| devices.sn            | string       | 设备序列号              |   |
	| devices.kind          | string       | 设备类型                | 保安分机为securityDev，五方对讲APP为fiveWayApp  |
	| devices.name          | string       | 设备名称                |   |
	| wechats               | object array | 小程序用户列表           |   |
	| wechats.userId        | string       | 小程序用户ID             |   |
	| wechats.name          | string       | 小程序用户昵称           |   |

#### 绑定设备

更新于：v1.1.3

设备发送至服务器。

* 请求
	```
	{
		"seq": 1,
		"cmd": "bindDevice",
		"modify": true,
		"sn": "11-22-33-AA-BB-CC",
		"kind": "fiveWayDev",
		"communityId": "COMMUNITYID",
		"building": "1",
		"unit": "1",
		"floor": "1",
		"name": "前门",
		"address": "ADDRESS",
		"callPlayEarlyMediaTimeout": 10,
		"callRingTimeout": 10,
		"callConverseTimeout": 10,
		"frequentlyCallPeriod": 600,
		"frequentlyCallTimes": 10,
		"callOrder": "sequential",
		"outgoingCalls": [{
			"kind": "device",
			"sn": "AA-BB-CC-11-22-33",
			"userId": ""
		}],
		"outgoingCallRescueTels": ["15915915911"],
		"channelNames": ["通道1"],
		"sipServerHost": "1.2.3.4",
		"sipServerPort": 5060,
		"sipUsername": "sip1",
		"sipPassword": "123456",
		"repairPressDuration": 5,
		"microphoneVolume": 100,
		"speakerVolume": 100
	}
	```

	| 参数                         | 类型          | 说明                   | 备注 |
	|------------------------------|--------------|------------------------|------|
	| modify                       | bool         | 是否修改设备            | true为修改，false为新增  |
	| sn                           | string       | 设备序列号              |   |
	| kind                         | string       | 设备类型                | 五方对讲机为fiveWayDev，五方对讲座机为fiveWaySecurity，保安分机为securityDev，单元机为unitDoor，围墙机为wallDoor  |
	| communityId                  | string       | 小区ID                  |   |
	| building                     | string       | 楼栋名                  | 可省略  |
	| unit                         | string       | 单元名                  | 可省略  |
	| floor                        | string       | 楼层                    | 可省略  |
	| name                         | string       | 设备名                  |   |
	| address                      | string       | 地址                    |   |
	| callPlayEarlyMediaTimeout    | int          | 呼叫播放早期媒体超时     | 单位为秒  |
	| callRingTimeout              | int          | 呼叫振铃超时            | 单位为秒  |
	| callConverseTimeout          | int          | 呼叫通话超时            | 单位为秒  |
	| frequentlyCallPeriod         | int          | 高频呼叫周期            | 单位为秒  |
	| frequentlyCallTimes          | int          | 高频呼叫次数限制         |   |
	| callOrder                    | string       | 呼叫顺序                | 顺振为sequential，同振为simultaneous  |
	| ~~outgoingCallDevices~~      | string array | 外呼设备序列号列表       |   |
	| outgoingCalls                | object array | 外呼列表                | 只当kind为fiveWayDev时使用  |
	| outgoingCalls.kind           | string       | 外呼类型                | 设备为device，小程序为wechat  |
	| outgoingCalls.sn             | string       | 外呼设备序列号           | 只当outgoingCalls.kind为device时使用  |
	| outgoingCalls.userId         | string       | 外呼小程序用户ID         | 只当outgoingCalls.kind为wechat时使用  |
	| outgoingCallRescueTels       | string array | 救援电话列表             |   |
	| channelNames                 | string array | 通道名列表               |   |
	| sipServerHost                | string       | SIP服务器地址           |   |
	| sipServerPort                | int          | SIP服务器端口           |   |
	| sipUsername                  | string       | SIP用户名               |   |
	| sipPassword                  | string       | SIP密码                 |   |
	| repairPressDuration          | bool         | 报修长按时长             | 单位为秒  |
	| microphoneVolume             | int          | 麦克风音量               |   |
	| speakerVolume                | int          | 扬声器音量               |   |
* 响应
	```
	{
		"seq": 1,
		"cmd": "bindDeviceAck",
		"result": "ok"
	
	}
	```

	如失败，需处理的reason：

	* this position has device
	* device is in another position

#### 获取小区五方通话设备列表

更新于：v1.1.3

设备发送至服务器。

* 请求
	```
	{
		"seq": 1,
		"cmd": "getFiveWayDevices",
		"communityId": "COMMUNITYID"
	}
	```

	| 参数                     | 类型          | 说明                   | 备注 |
	|--------------------------|--------------|------------------------|------|
	| communityId              | string       | 小区ID                  |   |
* 响应
	```
	{
		"seq": 1,
		"cmd": "getFiveWayDevicesAck",
		"result": "ok",
		"devices": [{
			"sn": "SN",
			"building": "1",
			"unit": "1",
			"name": "前门",
			"address": "ADDRESS",
			"imei": "IMEI",
			"iccid": "ICCID",
			"callPlayEarlyMediaTimeout": 10,
			"callRingTimeout": 10,
			"callConverseTimeout": 10,
			"frequentlyCallPeriod": 600,
			"frequentlyCallTimes": 10,
			"callOrder": "CALLORDER",
			"outgoingCallDevicesDetail": [{
				"outgoingKind": "device",
				"sn": "AA-BB-CC-11-22-33",
				"kind": "fiveWayApp",
				"userId": "",
				"name": "值班室"
			}],
			"outgoingCallRescueTels": ["15915915911"],
			"channelNames": ["通道1"],
			"sipServerHost": "1.2.3.4",
			"sipServerPort": 5060,
			"sipUsername": "sip1",
			"sipPassword": "123456",
			"sipAccounts": [{
				"serverHost": "1.2.3.4",
				"serverPort": 5060,
				"username": "USERNAME",
				"password": "PASSWORD"
			}],
			"repairPressDuration": 5,
			"microphoneVolume": 100,
			"speakerVolume": 100,
			"netStrength": -100,
			"netQuality": 100,
			"softwareVersion": "X9_7 (1.2.3)",
			"state": "online",
			"lastOnlineTime": "2006-01-02 15:04:05"
		}]
	}
	```

	| 参数                                           | 类型          | 说明                  | 备注 |
	|------------------------------------------------|--------------|-----------------------|------|
	| devices                                        | object array | 设备列表               |   |
	| devices.sn                                     | string       | 设备序列号             |   |
	| devices.building                               | string       | 楼栋名                 |   |
	| devices.unit                                   | string       | 单元名                 |   |
	| devices.name                                   | string       | 设备名                 |   |
	| devices.address                                | string       | 地址                   |   |
	| devices.imei                                   | string       | IMEI                   |   |
	| devices.iccid                                  | string       | ICCID                  |   |
	| devices.callPlayEarlyMediaTimeout              | int          | 呼叫播放早期媒体超时     | 单位为秒  |
	| devices.callRingTimeout                        | int          | 呼叫振铃超时            | 单位为秒  |
	| devices.callConverseTimeout                    | int          | 呼叫通话超时            | 单位为秒  |
	| devices.frequentlyCallPeriod                   | int          | 高频呼叫周期            | 单位为秒  |
	| devices.frequentlyCallTimes                    | int          | 高频呼叫次数限制         |   |
	| devices.callOrder                              | string       | 呼叫顺序                | 顺振为sequential，同振为simultaneous  |
	| ~~devices.outgoingCallDevices~~                | string array | 外呼设备序列号列表       |   |
	| devices.outgoingCallDevicesDetail              | object array | 外呼列表                |   |
	| devices.outgoingCallDevicesDetail.outgoingKind | string       | 外呼类型                | 设备为device，小程序为wechat  |
	| devices.outgoingCallDevicesDetail.sn           | string       | 外呼设备序列号           | 只当devices.outgoingCallDevicesDetail.outgoingKind为device时使用  |
	| devices.outgoingCallDevicesDetail.kind         | string       | 外呼设备类型             | 只当devices.outgoingCallDevicesDetail.outgoingKind为device时使用。保安分机为securityDev，五方对讲APP为fiveWayApp  |
	| devices.outgoingCallDevicesDetail.userId       | string       | 外呼小程序用户ID         | 只当devices.outgoingCallDevicesDetail.outgoingKind为wechat时使用  |
	| devices.outgoingCallDevicesDetail.name         | string       | 外呼设备名或小程序昵称    |   |
	| devices.outgoingCallRescueTels                 | string array | 救援电话列表             |   |
	| devices.channelNames                           | string array | 通道名列表               |   |
	| devices.sipServerHost                          | string       | SIP服务器地址           |   |
	| devices.sipServerPort                          | int          | SIP服务器端口           |   |
	| devices.sipUsername                            | string       | SIP用户名               |   |
	| devices.sipPassword                            | string       | SIP密码                 |   |
	| devices.sipAccounts                            | object array | SIP帐号列表              |   |
	| devices.sipAccounts.serverHost                 | string       | SIP服务器地址            |   |
	| devices.sipAccounts.serverPort                 | int          | SIP服务器端口            |   |
	| devices.sipAccounts.username                   | string       | SIP用户名                |   |
	| devices.sipAccounts.password                   | string       | SIP密码                  |   |
	| devices.repairPressDuration                    | int          | 报修长按时长              | 单位为秒  |
	| devices.microphoneVolume                       | int          | 麦克风音量                |   |
	| devices.speakerVolume                          | int          | 扬声器音量                |   |
	| devices.netStrength                            | int          | 网络信号强度              | 单位为dBm  |
	| devices.netQuality                             | int          | 网络质量                  | 0至100  |
	| devices.softwareVersion                        | string       | 设备软件版本              |   |
	| devices.state                                  | string       | 在线状态                  | 在线为online，离线为offline，未知为unknown  |
	| devices.lastOnlineTime                         | string       | 最后上线时间	            | 格式为2006-01-02 15:04:05  |

### 记录

#### 开锁记录 —— openRecord

设备发送至服务器。

* 请求
	```json
	{
		"time": "20060102150405",
		"kind": "KIND",
		"role": "ROLE",
		"card": "DDCCBBAA",
		"faceId": "FACEID",
		"userId": "USERID",
		"tel": "TEL",
		"sn": "SN",
		"opener": "OPENER",
		"withSnapshot": true,
		"snapshotContentType": "image/jpeg"
	}
	```

	| 参数                | 类型   | 说明               | 备注                                                                     |
	|---------------------|--------|--------------------|--------------------------------------------------------------------------|
	| time                | string | 开锁时间           | 格式为20060102150405                                                     |
	| kind                | string | 开锁类型           | 按键开锁为button，密码开锁为password，刷卡开锁为card，人脸开锁为face，微信开锁为wechat，二维码开锁（设备主扫）为qrcode，扫码开锁（设备被扫）为scan，五方对讲APP开锁为fiveWayApp，保安小叮分机开锁为securityDev，保安小叮为securityHandhold，室内手持设备为handhold，非法卡为illegalCard。可省略。如为“下发开锁”协议触发的开锁记录，则填写“下发开锁”协议请求的kind |
	| role                | string | 开锁人角色         | 物管为manager，住户为occupant，租客为tenant，访客为visitor，黑名单为deniedPerson。可省略。如为“下发开锁”协议触发的开锁记录，则填写“下发开锁”协议请求的role |
	| card                | string | 开锁物理卡号       | 大写或小写16进制，字节顺序为高位字节至低位字节。                           |
	| faceId              | string | 人脸注册ID         | 可省略                                                                   |
	| userId              | string | 开锁用户ID         | 可省略。如为“下发开锁”协议触发的开锁记录，则填写“下发开锁”协议请求的userId |
	| tel                 | string | 开锁手机号         | 可省略                                                                   |
	| sn                  | string | 开锁设备序列号     | 可省略。如为“下发开锁”协议触发的开锁记录，则填写“下发开锁”协议请求的sn     |
	| opener              | string | 开锁人             | 可省略。如为“下发开锁”协议触发的开锁记录，则填写“下发开锁”协议请求的opener |
	| withSnapshot        | bool   | 是否有抓拍图       |                                                                          |
	| snapshotContentType | string | 抓拍图MIME媒体类型 | 当withSnapshot为true时使用                                               |

	role、card、userId、opener根据kind值选填：

	* button: 无。
	* password: 无或role、userId。
	* card: role、card。
	* face: role、card或role、faceId。
	* wechat: role、userId、opener。
	* qrcode: 无。
	* scan: role、userId、opener。
	* securityDev: sn。
	* securityHandhold: sn。
	* handhold: sn。
	* illegalCard: card。
* 响应
	```json
	{
		"snapshotUpload": {
			"method": "PUT",
			"url": "https://domain.com/a/b/c.jpg",
			"headers": [{
				"key": "Content-Type",
				"value": "image/jpeg"
			}]
		}
	}
	```

	| 参数                         | 类型         | 说明                     | 备注                                  |
	|------------------------------|--------------|--------------------------|---------------------------------------|
	| snapshotUpload               | object       | 抓拍上传的方式           | 可省略。当请求withSnapshot为true时使用 |
	| snapshotUpload.method        | string       | 抓拍上传的HTTP方法       |                                       |
	| snapshotUpload.url           | string       | 抓拍上传的URL            |                                       |
	| snapshotUpload.headers       | object array | 抓拍上传的HTTP请求头列表 |                                       |
	| snapshotUpload.headers.key   | string       | 抓拍上传的HTTP请求头键名 |                                       |
	| snapshotUpload.headers.value | string       | 抓拍上传的HTTP请求头值   |                                       |

#### 呼叫记录 —— callRecord

更新于：v1.1.2

设备发送至服务器。

* 请求参数
	```json
	{
		"callId": "CALLID",
		"isCallee": false,
		"time": "20060102150405",
		"openTime": "20060102150405",
		"role": "occupant",
		"target": ["1", "2", "101"],
		"apartmentId": "APARTMENTID",
		"status": "accepted",
		"duration": 10,
		"acceptKind": "wechat",
		"userId": "USERID",
		"sn": "",
		"accepter": "",
		"snapshotUploadUrl": "https://domain.com/a/b/c.jpg"
	}
	```

	| 参数              | 类型         | 说明             | 备注                                 |
	|-------------------|--------------|------------------|--------------------------------------|
	| callId            | string       | 呼叫ID           |                                      |
	| ~~isCallee~~      | bool         | 是否被叫上传     |                                      |
	| time              | string       | 呼叫时间         | 格式为20060102150405                 |
	| openTime          | string       | 开锁时间         | 可省略。格式为20060102150405          |
	| role              | string       | 呼叫目标角色     | 物管为manager，住户为occupant         |
	| target            | string array | 呼叫目标按键序列 |                                      |
	| apartmentId       | string       | 呼叫住房ID       | 可省略。如role不为occupant，则忽略     |
	| status            | string       | 呼叫状态         | 已接听为accepted，未接听为unaccepted  |
	| duration          | int          | 呼叫时长         | 单位为秒。如status不为accepted，则忽略 |
	| acceptKind        | string       | 接听类型         |                                      |
	| userId            | string       | 接听用户ID       | 可省略                               |
	| sn                | string       | 接听设备序列号   | 可省略                               |
	| accepter          | string       | 接听人           | 可省略                               |
	| snapshotUploadUrl | string       | 抓拍上传的URL    |                                      |
* 响应参数

#### 报警记录 —— alarmRecord

设备请求服务器。

* 请求
    ```json
    {
        "startTime": 1136185445,
        "kind": "motion",
        "videoDuration": 60,
		"snapshots": [{
			"media": "image",
			"upload": true
		}]
    }
    ```

    | 参数             | 类型         | 说明         | 备注                    |
    |------------------|--------------|--------------|-------------------------|
    | startTime        | int64        | 开始时间戳   | 秒级                    |
    | kind             | string       | 报警类型     | 见下文描述              |
    | videoDuration    | int          | 录像时长     | 可省略。单位为秒         |
    | snapshots        | object array | 抓拍列表     |                         |
    | snapshots.media  | string       | 抓拍媒体类型 | 图片为image，视频为video |
    | snapshots.upload | bool         | 抓拍是否上传 |                         |

    kind可取的具体值如下：

	* doorContact：门磁触发。
	* loitering：长时间停留。
	* lowBattery：电池电量低。
	* motion：移动侦测。
* 响应
    ```json
    {
		"id": "ID",
		"snapshots": [{
			"media": "image",
			"method": "PUT",
			"url": "https://domain.com/a/b/c.jpg",
			"headers": [{
				"key": "Content-Type",
				"value": "image/jpeg"
			}]
		}]
    }
    ```

    | 参数                    | 类型         | 说明                     | 备注                      |
    |-------------------------|--------------|--------------------------|---------------------------|
    | snapshots               | object array | 抓拍列表                 |                           |
    | snapshots.media         | string       | 媒体类型                 | 图片为image，音视频为video |
    | snapshots.method        | string       | 文件上传的HTTP方法       |                           |
    | snapshots.url           | string       | 文件上传的URL            |                           |
    | snapshots.headers       | object array | 文件上传的HTTP请求头列表 |                           |
    | snapshots.headers.key   | string       | 文件上传的HTTP请求头键名 |                           |
    | snapshots.headers.value | string       | 文件上传的HTTP请求头值   |                           |

#### 报警记录更新 —— alarmRecordUpdate

设备请求服务器。

* 请求
	```json
	{
		"id": "ID",
		"videoDuration": 60
	}
	```

	| 参数          | 类型   | 说明     | 备注            |
	|---------------|--------|----------|-----------------|
	| id            | string | 记录ID   |                 |
	| videoDuration | int    | 录像时长 | 可省略。单位为秒 |

	可更新部分字段，未更新的字段省略（亦不为null）。
* 响应

#### 获取报警记录列表

更新于：v1.1.3

设备发送至服务器。

* 请求参数
	```
	{
		"seq": 1,
		"cmd": "getAlarmRecords",
		"page": 1,
		"amount": 10
	}
	```

	| 参数            | 类型          | 说明               | 备注    |
	|-----------------|--------------|--------------------|---------|
	| page            | int          | 页码                | 最大100 |
	| amount          | int          | 每页数量            | 最大100 |
* 响应参数
	```
	{
		"seq": 1,
		"cmd": "getAlarmRecordsAck",
		"result": "ok",
		"records": [{
			"id": "ID",
			"sn": "11-22-33-AA-BB-CC",
			"kind": "peopleTrapped",
			"time": "2006-01-02 15:04:05",
			"position": "美丽花园1栋1单元前门"
		}]
	}
	```

	| 参数                 | 类型          | 说明               | 备注    |
	|----------------------|--------------|--------------------|---------|
	| records              | object array | 报警记录列表        |   |
	| records.id           | string       | 报警记录ID          |   |
	| records.sn           | string       | 报警设备序列号      |   |
	| records.kind         | string       | 报警类型            |   |
	| records.time         | string       | 报警时间            | 格式为2006-01-02 15:04:05  |
	| records.position     | string       | 报警位置            |   |

### 回放

#### 抓取回放日期列表 —— requirePlaybackDates

服务器请求设备。

* 请求
    ```json
    {
        "channel": 1,
        "ipc": "192.168.1.1",
        "year": 2006,
        "month": 1
    }
    ```

    | 参数    | 类型   | 说明          | 备注                            |
    |---------|--------|---------------|---------------------------------|
    | channel | int    | 设备通道号    | 可省略。channel和ipc最多使用其一 |
    | ipc     | string | IPC摄像机地址 | 可省略。channel和ipc最多使用其一 |
    | year    | int    | 年            |                                 |
    | month   | int    | 月            |                                 |
* 响应
    ```json
    {
        "dates": [{
            "year": 2006,
            "month": 1,
            "date": 2
        }]
    }
    ```

    | 参数        | 类型         | 说明     | 备注         |
    |-------------|--------------|----------|--------------|
    | dates       | object array | 日期列表 | 依次从早到晚 |
    | dates.year  | int          | 年       |              |
    | dates.month | int          | 月       |              |
    | dates.date  | int          | 日       |              |

#### 抓取回放列表 —— requirePlaybacks

服务器请求设备。

* 请求
    ```json
    {
        "channel": 1,
        "ipc": "192.168.1.1",
        "year": 2006,
        "month": 1,
        "date": 2
    }
    ```

    | 参数    | 类型   | 说明          | 备注                            |
    |---------|--------|---------------|---------------------------------|
    | channel | int    | 设备通道号    | 可省略。channel和ipc最多使用其一 |
    | ipc     | string | IPC摄像机地址 | 可省略。channel和ipc最多使用其一 |
    | year    | int    | 年            |                                 |
    | month   | int    | 月            |                                 |
    | date    | int    | 日            |                                 |
* 响应
    ```json
    {
        "playbacks": [{
            "startTime": 1136214245,
            "endTime": 1136214245,
            "media": "video",
            "kind": "motion"
        }]
    }
    ```

    | 参数                | 类型         | 说明         | 备注                      |
    |---------------------|--------------|--------------|---------------------------|
    | playbacks           | object array | 录像回放列表 | 依次从早到晚              |
    | playbacks.startTime | int64        | 开始时间戳   | 秒级                      |
    | playbacks.endTime   | int64        | 结束时间戳   | 秒级                      |
    | playbacks.media     | string       | 媒体类型     | 图片为image，音视频为video |
    | playbacks.kind      | string       | 回放类型     | 见下文描述                |

	playbacks.kind可取的具体值如下：

	* call：呼叫。
	* doorContact：门磁触发。
	* loitering：长时间停留。
	* monitor：监视。
	* motion：移动侦测。

#### 抓取回放文件 —— requirePlaybackFile

服务器请求设备。

* 请求
    ```json
    {
		"startTime": 1136214245,
		"media": "video",
		"method": "PUT",
		"url": "https://domain.com/a/b/c.jpg",
		"headers": [{
			"key": "Content-Type",
			"value": "image/jpeg"
		}]
    }
    ```

    | 参数          | 类型         | 说明                     | 备注                      |
    |---------------|--------------|--------------------------|---------------------------|
    | startTime     | int64        | 开始时间戳               | 秒级                      |
    | media         | string       | 媒体类型                 | 图片为image，音视频为video |
    | method        | string       | 文件上传的HTTP方法       |                           |
    | url           | string       | 文件上传的URL            |                           |
    | headers       | object array | 文件上传的HTTP请求头列表 |                           |
    | headers.key   | string       | 文件上传的HTTP请求头键名 |                           |
    | headers.value | string       | 文件上传的HTTP请求头值   |                           |
* 响应

#### 抓取回放文件反馈 —— requirePlaybackFileFeedback

设备请求服务器。

* 请求
    ```json
    {
		"startTime": 1136214245,
		"media": "video",
		"success": true
    }
    ```

    | 参数      | 类型   | 说明             | 备注                      |
    |-----------|--------|------------------|---------------------------|
    | startTime | int64  | 开始时间戳       | 秒级                      |
    | media     | string | 媒体类型         | 图片为image，音视频为video |
    | success   | bool   | 文件上传是否成功 |                           |
* 响应

### 遮罩

#### 抓拍 —— requireSnapshot

服务器请求设备。

* 请求
    ```json
    {
		"method": "PUT",
		"url": "https://domain.com/a/b/c.jpg",
		"headers": [{
			"key": "Content-Type",
			"value": "image/jpeg"
		}]
    }
    ```

    | 参数          | 类型         | 说明                     | 备注 |
    |---------------|--------------|--------------------------|------|
    | method        | string       | 图片上传的HTTP方法       |      |
    | url           | string       | 图片上传的URL            |      |
    | headers       | object array | 图片上传的HTTP请求头列表 |      |
    | headers.key   | string       | 图片上传的HTTP请求头键名 |      |
    | headers.value | string       | 图片上传的HTTP请求头值   |      |
* 响应

#### 抓拍反馈 —— requireSnapshotFeedback

设备请求服务器。

* 请求
    ```json
    {
		"success": true
    }
    ```

    | 参数    | 类型 | 说明             | 备注 |
    |---------|------|------------------|------|
    | success | bool | 图片上传是否成功 |      |
* 响应

### 授权码

#### 客户编号获取

更新于：v1.1.3

设备发送至服务器。

* 请求
	```
	{
		"seq": 1,
		"cmd": "customerCodeGet"
	}
	```
* 响应
	```
	{
		"seq": 1,
		"cmd": "customerCodeGetAck",
		"result": "ok",
		"licenseCustomerCode": "456789",
		"execTime": 1665331200
	}
	```

	| 参数                | 类型   | 说明               | 备注 |
	|---------------------|--------|------------------|------|
	| licenseCustomerCode | string | 授权码系统客户编号 |      |
	| execTime            | int64  | 执行秒级时间戳     |      |

#### 获取服务

更新于：v1.1.3

设备发送至服务器。

* 请求
	```
	{
		"seq": 1,
		"cmd": "getServices"
	}
	```
* 响应
	```
	{
		"seq": 1,
		"cmd": "getServicesAck",
		"result": "ok",
		"callServices": ["trtc"]
	}
	```

	| 参数                | 类型          | 说明                     | 备注  |
	|---------------------|--------------|--------------------------|-------|
	| callServices        | string array | 设备呼叫使用的服务列表     | trtc为TRTC音视频，voip为VoIP音视频  |

#### 获取单个腾讯IoT密钥

更新于：v1.1.2

设备发送至服务器。

* 请求
	```
	{
		"seq": 1,
		"cmd": "txIotGetSecretKey",
		"serviceKinds": ["trtc"]
	}
	```

	| 参数                   | 类型          | 说明                      | 备注  |
	|------------------------|--------------|---------------------------|-------|
	| serviceKinds           | string array | 服务类型                   | TRTC为trtc，VoIP摄像头为voipCamera，VoIP门禁为voipDoor |
* 响应
	```
	{
		"seq": 1,
		"cmd": "txIotGetSecretKeyAck",
		"result": "ok",
		"available": true,
		"txIotProductId": "TXIOTPRODUCTID",
		"txIotDeviceName": "TXIOTDEVICENAME",
		"txIotDevicePsk": "TXIOTDEVICEPSK",
		"wxAppId": "WXAPPID",
		"wxModelId": "WXMODELID",
		"wxSn": "WXSN",
		"wxTicket": "WXTICKET"
	}
	```

	| 参数              | 类型          | 说明                  | 备注  |
	|-------------------|--------------|-----------------------|-------|
	| available         | bool         | 是否可用               | 如为false，则忽略后面的字段  |
	| txIotProductId    | string       | 腾讯IoT设备产品ID      |   |
	| txIotDeviceName   | string       | 腾讯IoT设备名称        |   |
	| txIotDevicePsk    | string       | 腾讯IoT设备密钥        |   |
	| wxAppId           | string       | 微信小程序AppId        |   |
	| wxModelId         | string       | 微信硬件设备modelId    |   |
	| wxSn              | string       | 微信硬件设备sn         |   |
	| wxTicket          | string       | 微信硬件设备票据        |   |

	如失败，需处理的reason：

	* no license customer code
	* license exhausted

#### 获取所有腾讯IoT密钥

更新于：v1.1.3

设备发送至服务器。

* 请求
	```
	{
		"seq": 1,
		"cmd": "txIotGetSecretKeys",
		"kind": "door",
		"chipKind": "X9",
		"txIotDevices": [{
			"serviceKinds": ["trtc"]
		}],
		"webrtc": true,
		"webrtcPooling": true
	}
	```

	| 参数                      | 类型         | 说明                     | 备注                                                                                        |
	|---------------------------|--------------|--------------------------|---------------------------------------------------------------------------------------------|
	| kind                      | string       | 授权分配类型             | controlPanel为中控屏，door为门口机，ipc为摄像头，securityExtension为保安分机                   |
	| chipKind                  | string       | 芯片类型                 | 可为F3T、NX1S、NX5、V1、X9、X20                                                                  |
	| txIotDevices              | object array | 需要的腾讯IoT设备列表    |                                                                                             |
	| txIotDevices.serviceKinds | string array | 服务类型列表             | trtc为TRTC，voipCamera为VoIP摄像头，voipDoor为VoIP门禁。[A]与[A, B]代表的数据互相独立，并无关联 |
	| webrtc                    | bool         | 是否需要WebRTC           |                                                                                             |
	| webrtcPooling             | bool         | 是否使用池化WebRTC授权码 | 为true                                                                                      |
* 响应
	```
	{
		"seq": 1,
		"cmd": "txIotGetSecretKeysAck",
		"result": "ok",
		"txIotDevices": [{
			"serviceKinds": ["trtc"],
			"txIotProductId": "IKZ697CD0X",
			"txIotDeviceName": "11-22-33-44-55-66",
			"txIotDevicePsk": "E5C+CyalAc++97PIUVshqw==",
			"wxAppId": "wx88a00a4256171f42",
			"wxModelId": "6ETu0tsLo0wPp-RAD4-bzk",
			"wxSn": "11-22-33-44-55-66-door",
			"wxTicket": "29a50a41301357s2ee43b50ef409c382"
		}],
		"webrtc": {
			"host": "webrtc.domain.com",
			"initString": "BASE64"
		}
	}
	```

	| 参数                         | 类型         | 说明                          | 备注                                                                                        |
	|------------------------------|--------------|-------------------------------|---------------------------------------------------------------------------------------------|
	| ~~available~~                | bool         | 是否可用                      | 如为false，则忽略后面的字段                                                                  |
	| txIotDevices                 | object array | 创建的腾讯IoT设备列表         | 其数量不多于请求的txIotDevices，不同元素的serviceKinds可相同                                 |
	| txIotDevices.serviceKinds    | string array | 服务类型列表                  | trtc为TRTC，voipCamera为VoIP摄像头，voipDoor为VoIP门禁。[A]与[A, B]代表的数据互相独立，并无关联 |
	| txIotDevices.txIotProductId  | string       | 腾讯IoT设备产品ID             | txIotDevices.serviceKinds含有trtc/voipCamera/voipDoor之一时使用                             |
	| txIotDevices.txIotDeviceName | string       | 腾讯IoT设备名称               | txIotDevices.serviceKinds含有trtc/voipCamera/voipDoor之一时使用                             |
	| txIotDevices.txIotDevicePsk  | string       | 腾讯IoT设备密钥               | txIotDevices.serviceKinds含有trtc/voipCamera/voipDoor之一时使用                             |
	| txIotDevices.wxAppId         | string       | 微信小程序AppId               | txIotDevices.serviceKinds含有voipCamera/voipDoor之一时使用                                  |
	| txIotDevices.wxModelId       | string       | 微信硬件设备modelId           | txIotDevices.serviceKinds含有voipCamera/voipDoor之一时使用                                  |
	| txIotDevices.wxSn            | string       | 微信硬件设备sn                | txIotDevices.serviceKinds含有voipCamera/voipDoor之一时使用                                  |
	| txIotDevices.wxTicket        | string       | 微信硬件设备票据              | txIotDevices.serviceKinds含有voipCamera/voipDoor之一时使用                                  |
	| webrtc                       | object       | WebRTC                        | 可省略。即使请求的webrtc为true，也可能获取不到                                                |
	| webrtc.host                  | string       | WebRTC服务器主机              |                                                                                             |
	| webrtc.initString            | string       | WebRTC的设备SDK参数InitString |                                                                                             |

	如失败，需处理的reason：

	* no license customer code
	* license exhausted

#### 获取润华WebRTC授权码 —— getLicenseRhWebrtc

设备请求服务器。

* 请求
* 响应

    ```json
    {
        "rhAddr": "webrtc.domain.com",
        "rhLicense": "SDAK-00-A1B2-C3D4-00000001",
        "rhInitString": "INITSTRING"
    }
    ```

    | 参数         | 类型   | 说明                   | 备注 |
    |--------------|--------|----------------------|------|
    | rhAddr       | string | 润华WebRTC服务器地址   |      |
    | rhLicense    | string | 润华WebRTC授权码       |      |
    | rhInitString | string | 润华WebRTC的InitString |      |

### 呼叫

实现的功能如下：

* 呼叫方式：设备呼叫物管、设备呼叫房号、设备选择小程序呼叫、设备/小程序回呼设备。
* 呼叫顺序：同呼、轮呼。
* 媒体类型：音频、音视频、无音视频。
* 被呼方未接听时可彻底挂断所有被呼方的呼叫。
* 被呼方显示主呼方的抓拍图。
* 被呼方获取主呼方的信息：位置、是否可开锁、是否可呼梯、是否可消警。
* 五方通话的通道呼叫。
* 第三方API接入支持。
* 兼容现有的呼叫协议：使用RTMP的call协议簇，使用腾讯连连TRTC的txIotCall协议簇。
* 呼叫信令和媒体传输方式分离，易于更换媒体传输方式。
* 同一次呼叫中多个被呼方可使用不同的媒体传输方式。

设备间呼叫的协议交互：

```
CALLER              SERVER             CALLEE
|----callXInitiate--->|                     |
|----[POST image]---->|                     |
|----callXInvite----->|                     |
|                     |----callXInvited---->|
|<---callXReplied-----|                     |
|                     |--callXSnapshotted-->|
|..................waiting..................|
|                     |<---callXAccept------|
|<---callXAccepted----|                     |
|<---callXConfirmed---|---callXConfirmed--->|
|...............[audio/video]...............|
|                     |<---callXHangUp------|
|<---callXHungUp------|                     |
|----callXHangUp----->|                     |
|---callXTerminate--->|                     |
```

被呼为低功耗设备的协议交互：

```
CALLER              SERVER             CALLEE
|----callXInitiate--->|                     |
|----[POST image]---->|                     |
|----callXInvite----->|                     |
|                     |-------wakeUp------->|
|                     |<-------login--------|
|                     |----callXInvited---->|
|<---callXReplied-----|                     |
|                     |--callXSnapshotted-->|
|..................waiting..................|
|                     |<---callXAccept------|
|<---callXAccepted----|                     |
|<---callXConfirmed---|---callXConfirmed--->|
|...............[audio/video]...............|
|                     |<---callXHangUp------|
|<---callXHungUp------|                     |
|----callXHangUp----->|                     |
|---callXTerminate--->|                     |
```

CALLER的callXInitiate总是以callXTerminate结束。CALLER的每一轮callXInvite总是以callXHangUp结束。

CALLEE总是以callXHangUp或callXHungUp结束。

呼叫过程中的unableCause有：

* device offline：设备离线

#### 呼叫发起 —— callXInitiate

设备请求服务器。

* 请求
    ```json
    {
        "v": 2,
        "channel": 1,
        "withSnapshot": true,
        "snapshotContentType": "image/jpeg",
        "canOpen": true,
        "canLiftCall": false,
        "canCancelAlarm": false,
        "alarmId": "ALARMID",
        "supportedModes": ["voip", "callX"],
        "supportedMedias": ["video", "audio", "none"],
        "requiredMedias": ["video", "audio", "none"],
        "projectKind": "",
        "callKind": "visitor",
        "toDevice": false,
        "calleeDevices": [{
            "sn": "ABCD1234",
            "channel": 2
        }],
		"calleeUserIds": ["USERID"],
        "calleeConfigTable": "1010101010101",
        "toManager": false,
        "calleeLocationId": "CALLEELOCATIONID",
		"calleeApartment": "101"
    }
    ```

    | 参数                  | 类型         | 说明                       | 备注                                                                 |
    |-----------------------|--------------|----------------------------|----------------------------------------------------------------------|
    | v                     | int          | 协议子版本号               | 固定为2                                                              |
    | channel               | int          | 主呼设备通道号             | 可省略，部分设备使用                                                  |
    | withSnapshot          | bool         | 主呼是否有抓拍图           |                                                                      |
    | snapshotContentType   | string       | 抓拍图MIME媒体类型         | 当withSnapshot为true时使用                                           |
    | canOpen               | bool         | 主呼是否可被开锁           |                                                                      |
    | canLiftCall           | bool         | 主呼是否可被呼梯           |                                                                      |
    | canCancelAlarm        | bool         | 主呼是否可被消警           |                                                                      |
    | alarmId               | string       | 报警记录ID                 | 可省略                                                               |
    | supportedModes        | string array | 主呼支持的呼叫模式列表     | 优先级依次从高到低，可为callX、voip、webrtc。见下文描述                  |
    | supportedMedias       | string array | 主呼支持的发送媒体类型列表 | 优先级依次从高到低。audio为音频，none为无音视频，video为音视频          |
    | requiredMedias        | string array | 主呼要求的接收媒体类型列表 | 优先级依次从高到低。audio为音频，none为无音视频，video为音视频          |
    | projectKind           | string       | 项目类型                   | 可省略                                                               |
    | callKind              | string       | 呼叫类型                   | 当toDevice为true时可省略。monitor为监视，visitor为访客呼叫             |
    | toDevice              | bool         | 是否呼叫设备               |                                                                      |
    | calleeDevices         | object array | 被呼设备列表               |                                                                      |
    | calleeDevices.sn      | string       | 被呼设备序列号             |                                                                      |
    | calleeDevices.channel | int          | 被呼设备通道号             |                                                                      |
    | calleeUserIds         | string array | 被呼用户ID列表             |                                                                      |
    | calleeConfigTable     | string       | 被呼配置表值               |                                                                      |
    | toManager             | bool         | 是否呼叫物管               | 当callKind为visitor时使用                                            |
    | calleeLocationId      | string       | 被呼住户的位置ID           | 当callKind为visitor，且toManager为false时使用。此时也可省略，见下文说明 |
    | calleeApartment       | string       | 被呼住户的住房房号         | 当callKind为visitor，且toManager为false时使用。此时也可省略，见下文说明 |

    关于呼叫模式的说明：
    * callX：调用callX协议簇，使用腾讯连连的TRTC传输媒体。
    * voip：调用callX协议簇，使用腾讯连连和微信的VoIP传输媒体。
    * webrtc：调用callX协议簇，使用WebRTC传输媒体。

    关于业务场景的说明：
    * 呼叫设备：toDevice固定为true，必需使用calleeDevices。
    * 呼叫物管：toManager固定为true。
    * 呼叫住户：
        * 主呼为室内设备（及代理其它设备）：toManager固定为false。
        * 主呼为公区设备：
			* toManager固定为false，如主呼为单元设备则使用calleeApartment，否则还需使用calleeLocationId指定楼栋ID。
			* toManager固定为false，使用calleeConfigTable。
			* toManager固定为false，使用calleeUserIds。
* 响应
    ```json
    {
        "callId": "CALLID",
        "snapshotUpload": {
            "method": "PUT",
            "url": "https://domain.com/a/b/c.jpg",
            "headers": [{
                "key": "Content-Type",
                "value": "image/jpeg"
            }]
        },
        "rounds": [{
            "level": 1,
            "orderKind": "simultaneous",
            "callees": [{
                "kind": "device",
                "sn": "ABCD1234",
                "userId": "",
                "webUsername": "",
                "webSessionId": "",
                "tel": "",
                "alias": "1号梯",
                "supportedModes": ["voip", "callX"],
                "wxOpenId": "",
                "voipPriority": 0,
                "able": true,
                "unableCause": ""
            }],
            "ringTimeout": 10,
            "converseTimeout": 20
        }],
		"calleeApartmentId": "CALLEEAPARTMENTID"
    }
    ```

    | 参数                          | 类型         | 说明                     | 备注                                                    |
    |-------------------------------|--------------|--------------------------|---------------------------------------------------------|
    | callId                        | string       | 呼叫ID                   |                                                         |
    | snapshotUpload                | object       | 抓拍上传的方式           | 可省略                                                  |
    | snapshotUpload.method         | string       | 抓拍上传的HTTP方法       |                                                         |
    | snapshotUpload.url            | string       | 抓拍上传的URL            |                                                         |
    | snapshotUpload.headers        | object array | 抓拍上传的HTTP请求头列表 |                                                         |
    | snapshotUpload.headers.key    | string       | 抓拍上传的HTTP请求头键名 |                                                         |
    | snapshotUpload.headers.value  | string       | 抓拍上传的HTTP请求头值   |                                                         |
    | rounds                        | object array | 呼叫轮次列表             | 依次从先到后                                            |
    | rounds.level                  | int          | 呼叫层级                 |                                                         |
    | rounds.orderKind              | string       | 呼叫顺序类型             | sequential为轮呼，simultaneous为同呼                     |
    | rounds.callees                | object array | 被呼列表                 | 当rounds.orderKind为sequential时，依次从先到后           |
    | rounds.callees.kind           | string       | 被呼类型                 | app为APP，device为设备，tel为电话，web为Web，wechat为小程序 |
    | rounds.callees.sn             | string       | 被呼设备序列号           | 当rounds.callees.kind为device时使用                     |
    | rounds.callees.userId         | string       | 被呼小程序用户ID         | 当rounds.callees.kind为app、wechat时使用                 |
    | rounds.callees.webUsername    | string       | 被呼Web登录用户名        |                                                         |
    | rounds.callees.webSessionId   | string       | 被呼Web会话ID            |                                                         |
    | rounds.callees.tel            | string       | 被呼电话                 | 当rounds.callees.kind为tel时使用                        |
    | rounds.callees.alias          | string       | 被呼的显示别名           |                                                         |
    | rounds.callees.supportedModes | string array | 被呼支持的呼叫模式列表   | 优先级依次从高到低，可为callX、voip、webrtc                |
    | rounds.callees.wxOpenId       | string       | 被呼小程序微信openId     | 当rounds.callees.kind为wechat，且rounds.callees.supportedModes有voip时使用 |
    | rounds.callees.voipPriority   | int          | 被呼小程序VoIP呼叫优先级 | 当rounds.orderKind为simultaneous、rounds.callees.kind为wechat，且rounds.callees.supportedModes有voip时使用。此时也可省略，优先级最低 |
    | rounds.callees.able           | bool         | 是否可以被呼             |                                                         |
    | rounds.callees.unableCause    | string       | 不可被呼的原因           | 当rounds.callees.able为false时使用，此时也可省略         |
    | rounds.ringTimeout            | int          | 振铃超时秒数             | 可省略                                                  |
    | rounds.converseTimeout        | int          | 通话超时秒数             | 可省略                                                  |
    | calleeApartmentId             | string       | 被呼住户的住房ID         |                                                         |

#### 呼叫邀请 —— callXInvite

设备请求服务器。

* 请求
    ```json
    {
        "v": 2,
        "callId": "CALLID",
        "ringTimeout": 10,
        "converseTimeout": 20,
        "round": 0,
        "callees": [{
            "kind": "device",
            "sn": "ABCD1234",
            "channel": 2,
            "userId": "",
            "webUsername": "",
            "webSessionId": "",
            "mode": "callX",
            "outOfBandUnderlyingCall": false
        }]
    }
    ```

    | 参数                            | 类型         | 说明                             | 备注                                            |
    |---------------------------------|--------------|--------------------------------|-------------------------------------------------|
    | v                               | int          | 协议子版本号                     | 固定为2                                         |
    | callId                          | string       | 呼叫ID                           |                                                 |
    | ringTimeout                     | int          | 振铃超时秒数                     | 可省略，则超时由被呼方控制                       |
    | converseTimeout                 | int          | 通话超时秒数                     | 可省略，则超时由被呼方控制                       |
    | round                           | int          | 呼叫轮次                         | 从0开始                                         |
    | callees                         | object array | 被呼列表                         |                                                 |
    | callees.kind                    | string       | 被呼类型                         | app为APP，device为设备，web为Web，wechat为小程序   |
    | callees.sn                      | string       | 被呼设备序列号                   | 当callees.kind为device时使用                    |
    | callees.channel                 | int          | 被呼设备通道号                   | 当callees.kind为device时使用，且只有部分设备使用 |
    | callees.userId                  | string       | 被呼小程序用户ID                 | 当callees.kind为app、wechat时使用                |
    | callees.webUsername             | string       | 被呼Web登录用户名                |                                                 |
    | callees.webSessionId            | string       | 被呼Web会话ID                    |                                                 |
    | callees.mode                    | string       | 被呼的呼叫模式                   | 可为callX、voip、webrtc                           |
    | callees.outOfBandUnderlyingCall | bool         | 被呼小程序是否使用带外的底层呼叫 | 当callees.kind为wechat且callees.mode为voip时使用。带外的底层呼叫，即设备在请求callXInitiate后，即可马上发起的底层呼叫，不再依赖后续协议 |
* 响应
    ```json
    {
        "callId": "CALLID"
    }
    ```

    | 参数                  | 类型          | 说明                   | 备注 |
    |-----------------------|--------------|------------------------|------|
    | callId                | string       | 呼叫ID                  |   |

#### 呼叫被邀请 —— callXInvited

服务器请求设备。

* 请求
    ```json
    {
        "callId": "CALLID",
        "mode": "callX",
        "supportedMedias": ["video", "audio", "none"],
        "requiredMedias": ["video", "audio", "none"],
        "canOpen": true,
        "canLiftCall": false,
        "canCancelAlarm": false,
        "alarmId": "ALARMID",
        "ringTimeout": 10,
        "converseTimeout": 20,
        "callerSn": "ABCD5678",
		"callerDeviceKind": "indoor",
        "callerCategoryLayers": ["EL", "S", "EL-S8-A0"],
        "callerIsSecondary": true,
        "callerChannel": 1,
        "callerUserId": "",
        "callerWebUsername": "",
        "callerWebSessionId": "",
        "callerApiUserId": "",
        "callerRole": "",
        "callerLocation": [{
            "name": "广东",
            "kind": "province"
        }],
        "callerAlias": "1号梯",
        "playback": true,
        "playbackTime": 1136214245,
        "calleeChannel": 2,
        "calleeIpc": "192.168.1.1",
        "snapshotDownloadUrl": "https://domain.com/a/b/c.jpg",
        "webrtcLicense": ""
    }
    ```

    | 参数                 | 类型         | 说明                       | 备注                                                              |
    |----------------------|--------------|----------------------------|-------------------------------------------------------------------|
    | callId               | string       | 呼叫ID                     |                                                                   |
    | mode                 | string       | 呼叫模式                   | 可为callX、voip、webrtc                                             |
    | supportedMedias      | string array | 主呼支持的发送媒体类型列表 | 优先级依次从高到低。audio为音频，none为无音视频，video为音视频       |
    | requiredMedias       | string array | 主呼要求的接收媒体类型列表 | 优先级依次从高到低。audio为音频，none为无音视频，video为音视频       |
    | canOpen              | bool         | 主呼是否可被开锁           |                                                                   |
    | canLiftCall          | bool         | 主呼是否可被呼梯           |                                                                   |
    | canCancelAlarm       | bool         | 主呼是否可被消警           |                                                                   |
    | alarmId              | string       | 报警记录ID                 |                                                                   |
    | ringTimeout          | int          | 振铃超时秒数               | 可省略，则超时由被呼方控制                                         |
    | converseTimeout      | int          | 通话超时秒数               | 可省略，则超时由被呼方控制                                         |
    | callerSn             | string       | 主呼设备序列号             | callerSn、callerUserId、callerWebUsername、callerApiUserId只使用其一 |
    | callerDeviceKind     | string       | 主呼设备类型               |                                                                   |
    | callerCategoryLayers | string array | 主呼设备产品类目层级列表   |                                                                   |
    | callerChannel        | int          | 主呼设备通道号             | 部分设备使用，与callerSn一起使用                                   |
    | callerIsSecondary    | bool         | 主呼设备是否为副机         |                                                                   |
    | callerUserId         | string       | 主呼小程序用户ID           | callerSn、callerUserId、callerWebUsername、callerApiUserId只使用其一 |
    | callerWebUsername    | string       | 主呼Web登录用户名          | callerSn、callerUserId、callerWebUsername、callerApiUserId只使用其一 |
    | callerWebSessionId   | string       | 主呼Web会话ID              | 与callerWebUsername一起使用                                       |
    | callerApiUserId      | string       | 主呼API小程序用户ID        | callerSn、callerUserId、callerWebUsername、callerApiUserId只使用其一 |
    | callerRole           | string       | 主呼身份                   | 可省略。manager为物管，occupant为住户                               |
    | callerLocation       | object array | 主呼位置层级列表           |                                                                   |
    | callerLocation.name  | string       | 主呼位置层级名称           |                                                                   |
    | callerLocation.kind  | string       | 主呼位置层级类型           |                                                                   |
    | callerAlias          | string       | 主呼的显示别名             |                                                                   |
    | playback             | bool         | 是否为录像回放             |                                                                   |
    | playbackTime         | int64        | 录像回放时间戳             | 秒级                                                              |
    | calleeChannel        | int          | 被呼设备通道号             | 可省略                                                            |
    | calleeIpc            | string       | 被呼IPC摄像机地址          | 可省略                                                            |
    | snapshotDownloadUrl  | string       | 下载抓拍图的URL            | 可省略                                                            |
    | webrtcLicense        | string       | 分配的webrtcLicense        | 可省略                                                            |

* 响应
    ```json
    {
        "callId": "CALLID",
        "able": true,
        "unableCause": "",
        "ringTimeout": 10,
        "converseTimeout": 20
    }
    ```

    | 参数            | 类型   | 说明           | 备注                       |
    |-----------------|--------|--------------|----------------------------|
    | callId          | string | 呼叫ID         |                            |
    | able            | bool   | 是否可以被呼   |                            |
    | unableCause     | string | 不可被呼的原因 | 当able为false时使用，可省略 |
    | ringTimeout     | int    | 振铃超时秒数   |                            |
    | converseTimeout | int    | 通话超时秒数   |                            |

#### 呼叫获取被邀请信息 —— callXGetInvited

设备请求服务器。

* 请求
    ```json
    {
        "callId": "CALLID"
    }
    ```
* 响应
    ```json
    {
        "callId": "CALLID",
        "mode": "callX",
        "supportedMedias": ["video", "audio", "none"],
        "requiredMedias": ["video", "audio", "none"],
        "canOpen": true,
        "canLiftCall": false,
        "canCancelAlarm": false,
        "alarmId": "ALARMID",
        "ringTimeout": 10,
        "converseTimeout": 20,
        "callerSn": "ABCD5678",
        "callerCategoryLayers": ["EL", "S", "EL-S8-A0"],
        "callerIsSecondary": true,
        "callerChannel": 1,
        "callerUserId": "",
        "callerWebUsername": "",
        "callerWebSessionId": "",
        "callerApiUserId": "",
        "callerRole": "",
        "callerLocation": [{
            "name": "广东",
            "kind": "province"
        }],
        "callerAlias": "1号梯",
        "playback": true,
        "playbackTime": 1136214245,
        "calleeChannel": 2,
        "calleeIpc": "192.168.1.1",
        "snapshotDownloadUrl": "https://domain.com/a/b/c.jpg",
        "webrtcLicense": ""
    }
    ```

    | 参数                 | 类型         | 说明                       | 备注                                                              |
    |----------------------|--------------|----------------------------|-------------------------------------------------------------------|
    | callId               | string       | 呼叫ID                     |                                                                   |
    | mode                 | string       | 呼叫模式                   | 可为callX、voip、webrtc                                             |
    | supportedMedias      | string array | 主呼支持的发送媒体类型列表 | 优先级依次从高到低。audio为音频，none为无音视频，video为音视频       |
    | requiredMedias       | string array | 主呼要求的接收媒体类型列表 | 优先级依次从高到低。audio为音频，none为无音视频，video为音视频       |
    | canOpen              | bool         | 主呼是否可被开锁           |                                                                   |
    | canLiftCall          | bool         | 主呼是否可被呼梯           |                                                                   |
    | canCancelAlarm       | bool         | 主呼是否可被消警           |                                                                   |
    | alarmId              | string       | 报警记录ID                 |                                                                   |
    | ringTimeout          | int          | 振铃超时秒数               | 可省略，则超时由被呼方控制                                         |
    | converseTimeout      | int          | 通话超时秒数               | 可省略，则超时由被呼方控制                                         |
    | callerSn             | string       | 主呼设备序列号             | callerSn、callerUserId、callerWebUsername、callerApiUserId只使用其一 |
    | callerCategoryLayers | string array | 主呼设备产品类目层级列表   |                                                                   |
    | callerChannel        | int          | 主呼设备通道号             | 部分设备使用，与callerSn一起使用                                   |
    | callerIsSecondary    | bool         | 主呼设备是否为副机         |                                                                   |
    | callerUserId         | string       | 主呼小程序用户ID           | callerSn、callerUserId、callerWebUsername、callerApiUserId只使用其一 |
    | callerWebUsername    | string       | 主呼Web登录用户名          | callerSn、callerUserId、callerWebUsername、callerApiUserId只使用其一 |
    | callerWebSessionId   | string       | 主呼Web会话ID              | 与callerWebUsername一起使用                                       |
    | callerApiUserId      | string       | 主呼API小程序用户ID        | callerSn、callerUserId、callerWebUsername、callerApiUserId只使用其一 |
    | callerRole           | string       | 主呼身份                   | 可省略。manager为物管，occupant为住户                               |
    | callerLocation       | object array | 主呼位置层级列表           |                                                                   |
    | callerLocation.name  | string       | 主呼位置层级名称           |                                                                   |
    | callerLocation.kind  | string       | 主呼位置层级类型           |                                                                   |
    | callerAlias          | string       | 主呼的显示别名             |                                                                   |
    | playback             | bool         | 是否为录像回放             |                                                                   |
    | playbackTime         | int64        | 录像回放时间戳             | 秒级                                                              |
    | calleeChannel        | int          | 被呼设备通道号             | 可省略                                                            |
    | calleeIpc            | string       | 被呼IPC摄像机地址          | 可省略                                                            |
    | snapshotDownloadUrl  | string       | 下载抓拍图的URL            | 可省略                                                            |
    | webrtcLicense        | string       | 分配的webrtcLicense        | 可省略                                                            |

#### 呼叫被响应 —— callXReplied

服务器请求设备。

* 请求
    ```json
    {
        "callId": "CALLID",
        "calleeSn": "ABCD1234",
        "calleeUserId": "",
        "calleeWebUsername": "",
        "calleeWebSessionId": "",
        "calleeApiUserId": "",
        "able": true,
        "unableCause": "",
        "ringTimeout": 10,
        "converseTimeout": 20
    }
    ```

    | 参数               | 类型   | 说明                  | 备注                                                              |
    |--------------------|--------|---------------------|-------------------------------------------------------------------|
    | callId             | string | 呼叫ID                |                                                                   |
    | calleeSn           | string | 被呼设备序列号        | calleeSn、calleeUserId、calleeWebUsername、calleeApiUserId只使用其一 |
    | calleeUserId       | string | 被呼小程序用户ID      | calleeSn、calleeUserId、calleeWebUsername、calleeApiUserId只使用其一 |
    | calleeWebUsername  | string | 被呼Web登录用户名     | calleeSn、calleeUserId、calleeWebUsername、calleeApiUserId只使用其一 |
    | calleeWebSessionId | string | 被呼Web会话ID         | 与calleeWebUsername一起使用                                       |
    | calleeApiUserId    | string | 被呼API小程序用户标识 | calleeSn、calleeUserId、calleeWebUsername、calleeApiUserId只使用其一 |
    | able               | bool   | 是否可以被呼          |                                                                   |
    | unableCause        | string | 不可被呼的原因        | 当able为false时使用，可省略                                        |
    | ringTimeout        | int    | 振铃超时秒数          |                                                                   |
    | converseTimeout    | int    | 通话超时秒数          |                                                                   |
* 响应
    ```json
    {
        "callId": "CALLID"
    }
    ```

    | 参数   | 类型   | 说明   | 备注 |
    |--------|--------|------|------|
    | callId | string | 呼叫ID |      |

部分被呼方不会触发本协议。

#### 呼叫被抓拍 —— callXSnapshotted

服务器请求设备。

* 请求
    ```json
    {
        "callId": "CALLID",
        "snapshotDownloadUrl": "https://domain.com/a/b/c.jpg"
    }
    ```

    | 参数                | 类型   | 说明            | 备注 |
    |---------------------|--------|---------------|------|
    | callId              | string | 呼叫ID          |      |
    | snapshotDownloadUrl | string | 下载抓拍图的URL |      |
* 响应
    ```json
    {
        "callId": "CALLID"
    }
    ```

    | 参数   | 类型   | 说明   | 备注 |
    |--------|--------|------|------|
    | callId | string | 呼叫ID |      |

#### 呼叫接听 —— callXAccept

设备请求服务器。

* 请求
    ```json
    {
        "callId": "CALLID",
        "callerMedia": "video",
        "calleeMedia": "video",
        "reverseUnderlyingCall": true,
        "canOpen": true,
        "canLiftCall": false,
        "canCancelAlarm": false,
        "calleeChannel": 2,
        "calleeIpc": "192.168.1.1"
    }
    ```

    | 参数                  | 类型   | 说明                   | 备注                                     |
    |-----------------------|--------|------------------------|------------------------------------------|
    | callId                | string | 呼叫ID                 |                                          |
    | callerMedia           | string | 主呼发送的媒体类型     | audio为音频，none为无音视频，video为音视频 |
    | calleeMedia           | string | 被呼发送的媒体类型     | audio为音频，none为无音视频，video为音视频 |
    | reverseUnderlyingCall | bool   | 是否使用反向的底层呼叫 | 当呼叫模式为callX时使用                  |
    | canOpen               | bool   | 被呼是否可被开锁       |                                          |
    | canLiftCall           | bool   | 被呼是否可被呼梯       |                                          |
    | canCancelAlarm        | bool   | 被呼是否可被消警       |                                          |
    | calleeChannel         | int    | 被呼设备通道号         | 部分设备使用                             |
    | calleeIpc             | string | 被呼IPC摄像机地址      | 呼叫IPC摄像机时使用                      |
* 响应
    ```json
    {
        "callId": "CALLID"
    }
    ```

    | 参数   | 类型   | 说明   | 备注 |
    |--------|--------|------|------|
    | callId | string | 呼叫ID |      |

#### 呼叫被接听 —— callXAccepted

服务器请求设备。

* 请求
    ```json
    {
        "callId": "CALLID",
        "callerMedia": "video",
        "calleeMedia": "video",
        "reverseUnderlyingCall": true,
        "acceptKind": "app",
        "canOpen": true,
        "canLiftCall": false,
        "canCancelAlarm": false,
        "calleeSn": "ABCD1234",
        "calleeChannel": 2,
        "calleeUserId": "",
        "calleeWebUsername": "",
        "calleeWebSessionId": "",
        "calleeApiUserId": "",
        "calleeLocation": [{
            "name": "广东",
            "kind": "province"
        }],
        "calleeAlias": "我叫张3"
    }
    ```

    | 参数                  | 类型         | 说明                   | 备注                                                              |
    |-----------------------|--------------|----------------------|-------------------------------------------------------------------|
    | callId                | string       | 呼叫ID                 |                                                                   |
    | callerMedia           | string       | 主呼发送的媒体类型     | audio为音频，none为无音视频，video为音视频                          |
    | calleeMedia           | string       | 被呼发送的媒体类型     | audio为音频，none为无音视频，video为音视频                          |
    | reverseUnderlyingCall | bool         | 是否使用反向的底层呼叫 | 当呼叫模式为callX时使用                                           |
    | acceptKind            | string       | 接听类型               | 可省略。app为APP接听，door为门口机接听                              |
    | canOpen               | bool         | 被呼是否可被开锁       |                                                                   |
    | canLiftCall           | bool         | 被呼是否可被呼梯       |                                                                   |
    | canCancelAlarm        | bool         | 被呼是否可被消警       |                                                                   |
    | calleeSn              | string       | 被呼设备序列号         | calleeSn、calleeUserId、calleeWebUsername、calleeApiUserId只使用其一 |
    | calleeChannel         | int          | 被呼设备号             | 部分设备使用，与calleeSn一起使用                                   |
    | calleeUserId          | string       | 被呼小程序用户ID       | calleeSn、calleeUserId、calleeWebUsername、calleeApiUserId只使用其一 |
    | calleeWebUsername     | string       | 被呼Web登录用户名      | calleeSn、calleeUserId、calleeWebUsername、calleeApiUserId只使用其一 |
    | calleeWebSessionId    | string       | 被呼Web会话ID          | 与calleeWebUsername一起使用                                       |
    | calleeApiUserId       | string       | 被呼API小程序用户标识  | calleeSn、calleeUserId、calleeWebUsername、calleeApiUserId只使用其一 |
    | calleeLocation        | object array | 被呼位置层级列表       |                                                                   |
    | calleeLocation.name   | string       | 被呼位置层级名称       |                                                                   |
    | calleeLocation.kind   | string       | 被呼位置层级类型       |                                                                   |
    | calleeAlias           | string       | 被呼的显示别名         |                                                                   |
* 响应
    ```json
    {
        "callId": "CALLID",
        "calleeSn": "ABCD1234",
        "calleeUserId": "",
        "calleeWebUsername": "",
        "calleeWebSessionId": "",
        "calleeApiUserId": "",
        "able": true,
        "unableCause": "",
        "hangUpOtherCallees": true
    }
    ```

    | 参数               | 类型   | 说明                  | 备注                                                              |
    |--------------------|--------|---------------------|-------------------------------------------------------------------|
    | callId             | string | 呼叫ID                |                                                                   |
    | calleeSn           | string | 被呼设备序列号        | calleeSn、calleeUserId、calleeWebUsername、calleeApiUserId只使用其一 |
    | calleeUserId       | string | 被呼小程序用户ID      | calleeSn、calleeUserId、calleeWebUsername、calleeApiUserId只使用其一 |
    | calleeWebUsername  | string | 被呼Web登录用户名     | calleeSn、calleeUserId、calleeWebUsername、calleeApiUserId只使用其一 |
    | calleeWebSessionId | string | 被呼Web会话ID         | 与calleeWebUsername一起使用                                       |
    | calleeApiUserId    | string | 被呼API小程序用户标识 | calleeSn、calleeUserId、calleeWebUsername、calleeApiUserId只使用其一 |
    | able               | bool   | 是否可以被接听        |                                                                   |
    | unableCause        | string | 不可被接听的原因      | 当able为false时使用，可省略                                        |
    | hangUpOtherCallees | bool   | 是否挂断其它被呼方    | 当able为true时使用                                                |

#### 呼叫被确认 —— callXConfirmed

服务器请求设备。

* 请求
    ```json
    {
        "callId": "CALLID",
        "able": true,
        "unableCause": "",
        "mode": "callX",
        "oppositeTxIotProductId": "PRODUCTID",
        "oppositeTxIotDeviceName": "DEVICENAME",
        "oppositeTxIotUserId": "",
        "oppositeWebrtcLicense": "",
        "webrtcSessionId": ""
    }
    ```

    | 参数                    | 类型   | 说明                         | 备注                       |
    |-------------------------|--------|----------------------------|----------------------------|
    | callId                  | string | 呼叫ID                       |                            |
    | able                    | bool   | 是否可以被接听               |                            |
    | unableCause             | string | 不可被接听的原因             | 当able为false时使用，可省略 |
    | mode                    | string | 呼叫模式                     | 可为callX、voip、webrtc      |
    | oppositeTxIotProductId  | string | 对端设备腾讯连连产品ID       | 与mode结合使用，见下文描述  |
    | oppositeTxIotDeviceName | string | 对端设备腾讯连连设备名称     | 与mode结合使用，见下文描述  |
    | oppositeTxIotUserId     | string | 对端设备腾讯连连小程序用户ID | 与mode结合使用，见下文描述  |
    | oppositeWebrtcLicense   | string | 对端设备WebRTC授权码         | 与mode结合使用，见下文描述  |
    | webrtcSessionId         | string | WebRTC会话ID                 | 与mode结合使用，见下文描述  |

    不同的mode及对端，需使用不同的字段：
    * mode为callX：
        * 对端为设备：oppositeTxIotProductId、oppositeTxIotDeviceName。
        * 对端为小程序：oppositeTxIotUserId。
    * mode为voip：
        * 对端为小程序：
            * 微信强提醒：无。
            * p2p：无。
    * mode为webrtc：
        * 对端为设备：oppositeWebrtcLicense、webrtcSessionId。
        * 对端为Web：webrtcSessionId。
* 响应
    ```json
    {
        "callId": "CALLID"
    }
    ```

    | 参数   | 类型   | 说明   | 备注 |
    |--------|--------|------|------|
    | callId | string | 呼叫ID |      |

#### 呼叫挂断 —— callXHangUp

设备请求服务器。

* 请求
    ```json
    {
        "callId": "CALLID",
        "entirely": true,
        "callees": [{
            "kind": "",
            "sn": "",
            "userId": "",
            "webUsername": "",
            "webSessionId": ""
        }],
        "cause": ""
    }
    ```

    | 参数                 | 类型         | 说明               | 备注                                                                                         |
    |----------------------|--------------|------------------|----------------------------------------------------------------------------------------------|
    | callId               | string       | 呼叫ID             |                                                                                              |
    | entirely             | bool         | 是否把呼叫彻底挂断 | 当为true时会把所有主呼、被呼挂断。当为false时，主呼方只挂断callees指定的被呼方；被呼方只挂断自身 |
    | callees              | object array | 被呼列表           | 主呼方当entirely为false时使用                                                                |
    | callees.kind         | string       | 被呼类型           | app为APP，device为设备，web为Web，wechat为小程序                                                |
    | callees.sn           | string       | 被呼设备序列号     | 当callees.kind为device时使用                                                                 |
    | callees.userId       | string       | 被呼小程序用户ID   | 当callees.kind为app、wechat时使用                                                             |
    | callees.webUsername  | string       | 被呼Web登录用户名  |                                                                                              |
    | callees.webSessionId | string       | 被呼Web会话ID      |                                                                                              |
    | cause                | string       | 挂断原因           | 可省略                                                                                       |
* 响应
    ```json
    {
        "callId": "CALLID"
    }
    ```

    | 参数   | 类型   | 说明   | 备注 |
    |--------|--------|------|------|
    | callId | string | 呼叫ID |      |

只在以下情况需要发送本协议：

* 主呼方/被呼方主动挂断时。
* 主呼方因呼叫期间协议的判断需终止本轮呼叫时。

#### 呼叫被挂断 —— callXHungUp

服务器请求设备。

* 请求
    ```json
    {
        "callId": "CALLID",
        "entirely": true,
        "cause": "",
        "calleeSn": "ABCD1234",
        "calleeUserId": "",
        "calleeWebUsername": "",
        "calleeWebSessionId": "",
        "calleeApiUserId": ""
    }
    ```

    | 参数               | 类型   | 说明                            | 备注                                                                  |
    |--------------------|--------|-------------------------------|-----------------------------------------------------------------------|
    | callId             | string | 呼叫ID                          |                                                                       |
    | entirely           | bool   | 是否把呼叫彻底挂断              | 主呼方使用，如为false，则只有指定的被呼方被挂断                         |
    | cause              | string | 挂断原因                        | 可省略                                                                |
    | calleeSn           | string | 发起挂断的被呼设备序列号        | calleeSn、calleeUserId、calleeWebUsername、calleeApiUserId最多只使用其一 |
    | calleeUserId       | string | 发起挂断的被呼小程序用户ID      | calleeSn、calleeUserId、calleeWebUsername、calleeApiUserId最多只使用其一 |
    | calleeWebUsername  | string | 发起挂断的被呼Web登录用户名     | calleeSn、calleeUserId、calleeWebUsername、calleeApiUserId最多只使用其一 |
    | calleeWebSessionId | string | 发起挂断的被呼Web会话ID         | 与calleeWebUsername一起使用                                           |
    | calleeApiUserId    | string | 发起挂断的被呼API小程序用户标识 | calleeSn、calleeUserId、calleeWebUsername、calleeApiUserId最多只使用其一 |
* 响应
    ```json
    {
        "callId": "CALLID"
    }
    ```

    | 参数   | 类型   | 说明   | 备注 |
    |--------|--------|------|------|
    | callId | string | 呼叫ID |      |

只在以下情况会收到本协议：

* 主呼方/被呼方发送callXHangUp时，对端会收到本协议。
* 主呼方回复callXAcceptedAck中hangUpOtherCallees为true时，其它被呼方会收到本协议。

#### 呼叫终止 —— callXTerminate

设备请求服务器。

* 请求
    ```json
    {
        "callId": "CALLID"
    }
    ```

    | 参数   | 类型   | 说明   | 备注 |
    |--------|--------|------|------|
    | callId | string | 呼叫ID |      |
* 响应
    ```json
    {
        "callId": "CALLID"
    }
    ```

    | 参数   | 类型   | 说明   | 备注 |
    |--------|--------|------|------|
    | callId | string | 呼叫ID |      |

### 开锁

#### 发起开锁 —— openAsk

设备请求服务器。

* 请求
	```json
	{
		"callId": "CALLID",
		"sn": "SN"
	}
	```

	| 参数   | 类型   | 说明           | 备注                        |
	|--------|--------|----------------|-----------------------------|
	| callId | string | 呼叫ID         | 可省略，在呼叫开锁时使用     |
	| sn     | string | 开锁设备序列号 | 当指定callId时，此值可能为空 |
* 响应

	如失败，需处理的reason：

	* device offline
	* open fail

#### 下发开锁 —— open

服务器发送至设备。

* 请求
	```json
	{
		"callId": "CALLID",
		"kind": "KIND",
		"role": "ROLE",
		"userId": "USERID",
		"sn": "SN",
		"opener": "OPENER",
		"floors": ["1"],
		"roomNumber": "101"
	}
	```

	| 参数       | 类型         | 说明           | 备注                                  |
	|------------|--------------|----------------|---------------------------------------|
	| callId     | string       | 呼叫ID         | 可省略。只当呼叫中开锁时使用           |
	| kind       | string       | 开锁类型       | 密码开锁为password，微信开锁为wechat，二维码开锁（设备主扫）为qrcode，扫码开锁（设备被扫）为scan，五方对讲APP开锁为fiveWayApp，保安分机开锁为securityHandhold，保安小叮开锁securityHandhold，室内手持设备为handhold。可省略。用于“开锁记录”协议请求的kind              |
	| role       | string       | 开锁人角色     | 超级管理员为admin，物管为manager，住户为occupant，租客为tenant，访客为visitor。可省略。用于“开锁记录”协议请求的role |
	| userId     | string       | 开锁用户ID     | 可省略。用于"开锁记录"协议请求的userId |
	| sn         | string       | 设备序列号     | 可省略。用于"开锁记录"协议请求的sn     |
	| opener     | string       | 开锁人         | 可省略。用于"开锁记录"协议请求的opener |
	| floors     | string array | 开锁人楼层列表 | 可省略                                |
	| roomNumber | string       | 开锁人房号     | 只当role为occupant时使用              |
* 响应

	如失败，需处理的reason：

	* no person info
	* no health info
	* abnormal health info
	* need to check health info

#### 校验二维码 —— checkQrcode

设备发送至服务器。

* 请求
	```json
	{
		"openQrcodeId": "OPENQRCODEID"
	}
	```

	| 参数         | 类型   | 说明         | 备注 |
	|--------------|--------|--------------|------|
	| openQrcodeId | string | 开锁二维码ID |      |
* 响应
	```json
	{
		"pass": true,
		"isManager": false,
		"rooms": [{
			"role": "ROLE",
			"floor": "1"
		}]
	}
	```

	| 参数        | 类型         | 说明         | 备注                        |
	|-------------|--------------|--------------|-----------------------------|
	| pass        | bool         | 是否校验通过 |                             |
	| isManager   | bool         | 是否为物管   |                             |
	| rooms       | object array | 房间列表     |                             |
	| rooms.role  | string       | 角色         | 住户为occupant，租客为tenant |
	| rooms.floor | string       | 楼层         |                             |

#### 校验密码 —— checkPassword

设备发送至服务器。

* 请求
	```json
	{
		"password": "PASSWORD"
	}
	```

	| 参数     | 类型   | 说明   | 备注 |
	|----------|--------|--------|------|
	| password | string | 开锁密码 |      |
* 响应
	```json
	{
		"pass": true
	}
	```

	| 参数 | 类型 | 说明         | 备注 |
	|------|------|--------------|------|
	| pass | bool | 是否校验通过 |      |

### 报警

#### 报警通知

更新于：v1.1.3

服务器发送至设备。

* 请求
	```
	{
		"seq": 1,
		"cmd": "alarmNotify",
		"id": "ID",
		"sn": "11-22-33-AA-BB-CC",
		"kind": "peopleTrapped",
		"time": "2006-01-02 15:04:05",
		"position": "美丽花园1栋1单元前门"
	}
	```

	| 参数     | 类型          | 说明               | 备注 |
	|----------|--------------|--------------------|------|
	| id       | string       | 报警记录ID          |   |
	| sn       | string       | 报警设备序列号      |   |
	| kind     | string       | 报警类型            |   |
	| time     | string       | 报警时间            | 格式为2006-01-02 15:04:05  |
	| position | string       | 报警位置            |   |
* 响应
	```
	{
		"seq": 1,
		"cmd": "alarmNotifyAck",
		"result": "ok"
	}
	```

#### 发起消警

更新于：v1.1.3

设备发送至服务器。

* 请求
	```
	{
		"seq": 1,
		"cmd": "alarmCancelAsk",
		"sn": "11-22-33-AA-BB-CC",
		"id": "ID"
	}
	```

	| 参数     | 类型          | 说明               | 备注 |
	|----------|--------------|--------------------|------|
	| sn       | string       | 设备序列号          |   |
	| id       | string       | 报警记录ID          |   |
* 响应
	```
	{
		"seq": 1,
		"cmd": "alarmCancelAskAck",
		"result": "ok"
	}
	```

#### 下发消警

更新于：v1.1.3

服务器发送至设备。

* 请求
	```
	{
		"seq": 1,
		"cmd": "alarmCancel",
		"id": "ID"
	}
	```

	| 参数     | 类型          | 说明               | 备注 |
	|----------|--------------|--------------------|------|
	| id       | string       | 报警记录ID          |   |
* 响应
	```
	{
		"seq": 1,
		"cmd": "alarmCancelAck",
		"result": "ok"
	}
	```

#### 消警

更新于：v1.1.3

设备发送至服务器。

* 请求
	```
	{
		"seq": 1,
		"cmd": "alarmCancelled",
		"id": "ID"
	}
	```

	| 参数     | 类型          | 说明               | 备注 |
	|----------|--------------|--------------------|------|
	| id       | string       | 报警记录ID          |   |
* 响应
	```
	{
		"seq": 1,
		"cmd": "alarmCancelledAck",
		"result": "ok"
	}
	```

### 远程控制

#### 下发重启 —— reboot

服务器请求设备。

* 请求
* 响应

#### 清除设备数据 —— erase

服务器请求设备。

* 请求
* 响应
* 说明

	服务器不会断开连接，设备需重新发送login才可上线。

#### 恢复出厂设置 —— reset

服务器请求设备。

* 请求
* 响应

#### 执行命令

更新于：v1.0.14

服务器发送至设备。

* 请求参数
	```
	{
		"seq": 1,
		"cmd": "exec",
		"command": "COMMAND"
	}
	```

	| 参数    | 类型   | 说明       | 备注 |
	|---------|--------|----------|------|
	| command | string | 命令协议名 | 需支持getUsers，reportInfo，reportSetting，reportStatus，sync，syncAd，syncResource，syncSetting，syncTime，txIotGetSecretKey，upgrade |
* 响应参数
	```
	{
		"seq": 1,
		"cmd": "execAck",
		"result": "ok",
		"command": "COMMAND"
	}
	```

	| 参数    | 类型   | 说明       | 备注                |
	|---------|--------|----------|-------------------|
	| command | string | 命令协议名 | 与请求的command一致 |

	不需等待命令执行，确认后立即回复。

## HTTP协议

### 获取二维码规则

* 请求
	```
	GET /bypass/qrcode/rule?code=98103239 HTTP/1.1
	Authorization: DpBypass AA-BB-CC-11-22-33
	```

	| 参数                  | 类型          | 说明                 | 备注 |
	|-----------------------|--------------|----------------------|------|
	| code                  | string       | 二维码编号            |   |
* 响应
	```
	HTTP/1.1 200 OK

	{
		"valid": true,
		"base": "https://xxx.com/weixin/miniprogram"
	}
	```

	| 参数                  | 类型          | 说明                 | 备注 |
	|-----------------------|--------------|----------------------|------|
	| valid                 | bool         | 二维码编号是否有效     |   |
	| base                  | string       | 基础规则              |   |

## 二维码

二维码内容使用PKCS#7填充的AES-CBC-256算法加密明文，以base64格式生成密文。

key为`m~Sal1GH0YU(xs%5ah&wYl(qS$VR^uuH`。
iv为`eOMejFQ%eNVZr&+j`。

如明文为t=b&sn=ABCD1234&m=EL-S8-A0，则密文为aNuLrZ79SFn99vomfBkjqipKLdzETkfCgnPNkfolViQ=。

明文格式见下述各小节。

### 设备绑定二维码

被扫。

```
t=b&sn=ABCD1234&m=EL-S8-A0&k=2&b=01&n=02
```

| 参数 | 类型   | 说明         | 备注                          |
|------|--------|--------------|-------------------------------|
| t    | string | 业务类型     | 固定为b                       |
| sn   | string | 设备序列号   |                               |
| m    | string | 产品类目型号 |                               |
| k    | string | 设备类型     | 1为室内机，2为单元机，7为围墙机 |
| b    | string | 楼栋号       |                               |
| n    | string | 设备号       |                               |

### 设备开锁二维码

被扫。

```
t=o&sn=ABCD1234&ts=1136185445
```

| 参数 | 类型   | 说明       | 备注                  |
|------|--------|------------|-----------------------|
| t    | string | 业务类型   | 固定为o               |
| sn   | string | 设备序列号 |                       |
| ts   | int    | 秒级时间戳 | 可省略。误差10分钟以内 |

### 设备临时开锁二维码

主扫。

```
t=ot&id=ID
```

| 参数 | 类型   | 说明         | 备注     |
|------|--------|--------------|----------|
| t    | string | 业务类型     | 固定为ot |
| id   | string | 开锁二维码ID |          |

### 设备系统设置二维码

主扫。

```
t=s&sn=ABCD1234&ts=1136185445
```

| 参数 | 类型   | 说明       | 备注           |
|------|--------|------------|----------------|
| t    | string | 业务类型   | 固定为s        |
| sn   | string | 设备序列号 |                |
| ts   | int    | 秒级时间戳 | 误差10分钟以内 |
