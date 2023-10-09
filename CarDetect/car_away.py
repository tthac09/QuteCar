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

car_device_id = "SportsCarB"

def procMQTTMsg(result):
    print("Begin to send msg!")
    if result == "RedCross":
        sendCMD(car_device_id, "CAR_CTRL", "CAR_AWAY", {"DURATION": 1000})


def sendCMD(device_id=car_device_id, service_id="TrafficLight", command_name="ControlModule",
            paras={"Light": "GREEN_LED_ON"}):
    ak = ""
    sk = ""
    project_id = ""
    region_id = "cn-north-4"
    endpoint = ".st1.iotda-app.cn-north-4.myhuaweicloud.com"

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
    procMQTTMsg("RedCross")
