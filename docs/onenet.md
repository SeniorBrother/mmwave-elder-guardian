# OneNet 云平台部署

数据上云用中国移动 OneNet 的「多协议接入 / MQTT」产品，数据点用物模型方式上报。

## 1. 注册与建产品

1. 打开 https://open.iot.10086.cn/ 注册登录。
2. 进入「多协议接入」→「MQTT」，创建产品：填名称、行业类别，联网方式选 WiFi，接入协议选 MQTT。
3. 产品创建后得到**产品 ID**（示例 `8w7dxp3Pj8`）。
4. 在产品下「添加设备」，设备名填 `dev1`，平台生成**设备密钥**。

## 2. 定义物模型（8 个标识符）

标识符必须与固件 JSON 的 key **完全一致、区分大小写**：

```
HeartRate  DHT11_T  DHT11_H  MLX90614  MQ7  People  Fall  RespiratoryRate
```

OTA 可再加一个 `Version` 上报固件版本。

## 3. MQTT 连接参数

| 项 | 值 |
|----|----|
| 服务器 IP | 183.230.40.96 |
| 端口 | 1883 |
| clientId | 设备名，如 dev1 |
| username | 产品 ID，如 8w7dxp3Pj8 |
| password | Token（见第 4 节） |
| 发布主题 | `$sys/{产品ID}/{设备名}/dp/post/json` |
| 订阅主题 | `$sys/{产品ID}/{设备名}/#` |

## 4. Token(password) 生成

Token 用账号 AccessKey 经 HMAC-SHA1 + Base64 算出，格式：

```
version=2018-10-31&res=products%2F{产品ID}&et={过期时间戳}&method=sha1&sign={签名}
```

用 `tools/onenet_token.py` 生成（把 AccessKey、产品ID、过期时间填进去）。**注意 et 过期后要重新生成。**

## 5. 上报 JSON 格式

```json
{
  "id": 123,
  "dp": {
    "HeartRate":       [{"v": 80}],
    "DHT11_T":         [{"v": 20}],
    "DHT11_H":         [{"v": 60}],
    "MLX90614":        [{"v": 36.2}],
    "MQ7":             [{"v": 10}],
    "People":          [{"v": 1}],
    "Fall":            [{"v": 0}],
    "RespiratoryRate": [{"v": 18}]
  }
}
```

固件侧由 `firmware/common/onenet_json.c` 的 `onenet_build_dp_json()` 生成。

## 6. 可视化网页

数据上报后进入 OneNet「可视化 View」（https://open.iot.10086.cn/studio/view/project ）：

1. 新建项目，选「试用专业版」。
2. 拖入仪表盘/数字卡片/开关状态组件。
3. 点组件「绑定数据流」，选产品与设备，绑定对应数据点。
4. 需要加单位/文字的组件写过滤器脚本，例如：

```javascript
return [{ "value": "环境温度: " + data[0].value + "℃" }];
```

有人/无人状态：

```javascript
if (data[0].value == 1) { return [{ "value": "老人状态: 在房间" }]; }
else { return [{ "value": "老人状态: 无人" }]; }
```

5. 排版后「发布」，生成手机/电脑可访问的分享链接。建议竖版、大字号，关键指标用红色预警阈值。

## 7. 常见问题

- **平台看不到数据**：九成是物模型标识符对不上（大小写、拼写），或发布主题写错。
- **MQTT 登录失败(收不到 20 02)**：检查产品ID/设备名/Token 是否与本账号一致，Token 是否过期。
- **上报频率**：演示可 2 秒一次，长期部署建议 10~30 秒，降低功耗与消息量（平台对频率有限制）。

FOTA 远程升级见 [fota.md](fota.md)。
