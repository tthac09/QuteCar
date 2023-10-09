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
import time

carA_device_id = "SportsCarA"
carB_device_id = "SportsCarB"
lightMain_device_id = "TrafficLightMain"
lightSub_device_id = "TrafficLightSub"
ambulance_device_id = "Ambulance"

def START():
    print("Begin to send msg!")
    sendCMD(lightMain_device_id, "TrafficLight", "ControlModule", {"Light": "GREEN_LED_ON"})
    sendCMD(carA_device_id, "CAR_CTRL", "TRACE_ON", {"DURATION": 0})
    sendCMD(carB_device_id, "CAR_CTRL", "TRACE_ON", {"DURATION": 0})
    time.sleep(5)
    sendCMD(ambulance_device_id, "CAR_CTRL", "TRACE_ON", {"DURATION": 0})


def sendCMD(device_id=carB_device_id, service_id="TrafficLight", command_name="ControlModule",
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
    START()
