# coding: utf-8

####
# 安装核心库
# pip install huaweicloudsdkcore

# 安装IoTDA服务库
# pip install huaweicloudsdkiotda
###
from huaweicloudsdkcore.exceptions import exceptions
from huaweicloudsdkcore.region.region import Region
from huaweicloudsdkiotda.v5 import *
from huaweicloudsdkcore.auth.credentials import BasicCredentials
from huaweicloudsdkcore.auth.credentials import DerivedCredentials

carB_device_id = "647848bff4d13061fc88addb_SportsCarB"
lightMain_device_id = "647848bff4d13061fc88addb_TrafficLightMain"

def procMQTTMsg1(result):
    print("Begin to send msg!")
    if result == "RedCross":
        sendCMD(carB_device_id, "CAR_CTRL", "CAR_AWAY", {"DURATION": 1000})


def procMQTTMsg2(result):
    print("Begin to send msg!")
    if result == "RedCross":
        sendCMD(lightMain_device_id, "TrafficLight", "ControlModule", {"Light": "RED_LED_ON"})

def sendCMD(device_id=carB_device_id, service_id="TrafficLight", command_name="ControlModule",
            paras={"Light": "GREEN_LED_ON"}):
    ak = "MZOLJXIKL7BYHIDJCPDD"
    sk = "6DrSd9ZDEK20TNX34oQlOcTuTQCorG0nlj5it84X"
    project_id = "a50c1d44bb81476fb42e1fee5a77712e"
    region_id = "cn-north-4"
    endpoint = "22cd56e00c.st1.iotda-app.cn-north-4.myhuaweicloud.com"

    REGION = Region(region_id, endpoint)

    credentials = BasicCredentials(ak, sk, project_id)

    credentials.with_derived_predicate(DerivedCredentials.get_default_derived_predicate())

    client = IoTDAClient.new_builder() \
        .with_credentials(credentials) \
        .with_region(REGION) \
        .build()
    try:
        request = CreateCommandRequest()
        request.device_id = device_id
        request.body = DeviceCommandRequest(
            paras=paras,
            command_name=command_name,
            service_id=service_id,

        )
        print(request)
        response = client.create_command(request)
        print(response)
    except exceptions.ClientRequestException as e:
        print(e.status_code)
        print(e.request_id)
        print(e.error_code)
        print(e.error_msg)


if __name__ == "__main__":
    procMQTTMsg1("RedCross")
