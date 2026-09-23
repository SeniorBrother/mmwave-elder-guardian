#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
OneNet MQTT 鉴权 Token(password) 生成脚本

Token 格式:
  version=2018-10-31&res={res}&et={et}&method={method}&sign={sign}
其中 sign = base64(hmac_sha1(key=base64decode(access_key), msg=待签名字符串))
待签名字符串 = "{et}\n{method}\n{res}\n{version}"

用法:
  python onenet_token.py <access_key> <product_id> [device_name] [et] [method]

示例:
  python onenet_token.py YOUR_ACCESS_KEY 8w7dxp3Pj8
  python onenet_token.py YOUR_ACCESS_KEY 8w7dxp3Pj8 dev1 1893456000 sha1
"""
import sys
import time
import base64
import hmac
import hashlib
from urllib.parse import quote

VERSION = "2018-10-31"


def gen_token(access_key, res, et=None, method="sha1", version=VERSION):
    """生成 OneNet 鉴权 Token。res 形如 products/{pid} 或 products/{pid}/devices/{dev}"""
    if et is None:
        # 默认 100 年后过期（可自行缩短）
        et = int(time.time()) + 100 * 365 * 24 * 3600
    et = str(et)

    # 待签名字符串: 各字段以 \n 连接，顺序 et, method, res, version
    org = et + "\n" + method + "\n" + res + "\n" + version

    # AccessKey 是 base64 编码的密钥，需先解码作为 HMAC 的 key
    key = base64.b64decode(access_key)

    if method == "sha1":
        digest = hmac.new(key, org.encode("utf-8"), hashlib.sha1).digest()
    elif method == "sha256":
        digest = hmac.new(key, org.encode("utf-8"), hashlib.sha256).digest()
    elif method == "md5":
        digest = hmac.new(key, org.encode("utf-8"), hashlib.md5).digest()
    else:
        raise ValueError("unsupported method: " + method)

    sign = base64.b64encode(digest).decode("utf-8")

    # res 和 sign 需 URL 编码
    token = "version={v}&res={r}&et={e}&method={m}&sign={s}".format(
        v=version, r=quote(res, safe=""), e=et, m=method, s=quote(sign, safe="")
    )
    return token, et


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    access_key = sys.argv[1]
    product_id = sys.argv[2]
    device_name = sys.argv[3] if len(sys.argv) > 3 else None
    et = sys.argv[4] if len(sys.argv) > 4 else None
    method = sys.argv[5] if len(sys.argv) > 5 else "sha1"

    if device_name:
        res = "products/{}/devices/{}".format(product_id, device_name)
    else:
        res = "products/{}".format(product_id)

    token, used_et = gen_token(access_key, res, et, method)

    print("res    :", res)
    print("method :", method)
    print("et     :", used_et)
    print("---- MQTT 参数 ----")
    print("clientId :", device_name if device_name else "自定义")
    print("username :", product_id)
    print("password :", token)


if __name__ == "__main__":
    main()
