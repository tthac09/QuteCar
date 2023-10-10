# QuteCar

This is an Engineering Innovation Practice Course Project of our group. We tried to develop a demo model of Intelligent Traffic System for Emergency Vehicles such as an ambulance or a fire truck. We named it as QuteCar after the word CUTE, which means we believe that BRIGHT future of Intelligent Traffic will make everyone love it.

这是我们团队的创新实践工程课程设计。我们尝试开发了一个为救护车或消防车等紧急车辆提供服务的智慧交通系统的演示模型，并用 Cute（可爱）一词将其命名为 QuteCar，意指我们相信智慧交通的光明未来将让每一个人喜欢它。

## Roadshow

You can find two videos on Page 20 of our slides RoadshowSlides.pptx, they show the the effectiveness of the system we design.

幻灯片 RoadshowSlides.pptx 的第 20 页有两个视频，他们展示了我们设计的系统的效果。

## Details

### Hardware

Three sets of HiSpark WiFi-IoT Smart Robot Car Developer Kit ([Link](www.hihope.org/en/pro/pro1.aspx?mtt=55)) produced by HiHope are used in place of three vehicles, one for a emergency vehicle named Ambulance, the others for private cars named SportA and SportB.

三套由 HiHope 润和生产的 HiSpark WiFi-IoT 智能小车开发套件（[链接](www.hihope.org/pro/pro1.aspx?mtt=55)）被用来模拟三辆车，其中一辆是被标记为 Ambulance 的紧急车辆，另两辆都是私家车，分别被标记为 SportA 和 SportB。

What's more, we design a housing with a Red Cross on it for Ambulance with 3D printing, in order to make it different from private cars, which makes it possible to identify Ambulance by computer vision.

并且，我们利用 3D 打印为 Ambulance 设计了一个带有红十字标志的外壳，以使其区别于私家车，从而模拟基于视觉的车辆识别。

Another two HiSpark WiFi-IoT Developer Kits are converted into two traffic lights with 3D printing and some other electronic components such as wires and LED lights. They are marked as TrafficLightMain and TrafficLightSub, and are controlled together usually but possible to controlled them seperately if needed.

通过使用 3D 打印技术和其他一些电子元件（例如导线和 LED 灯），另外两套 HiSpark WiFi-IoT 开发套件被改造为两台交通信号灯。他们被标记为 TrafficLightMain 和 TrafficLightSub，通常可以协同控制，但在需要时也可以独立控制。

Each Kit mentioned above uses a Hi3861 SoC supporting 2.4G WiFi. More
details and parameters about the SoC and Kit can be found on the link above and the website of HiSilicon ([link](www.hisilicon.com/en/products/smart-iot/ShortRangeWirelessIOT/Hi3861V100)).

所有套件都使用支持 2.4G WiFi 的 Hi3861 芯片。关于芯片和套件的更多细节和参数可以在上面的链接和海思的网站上找到（[链接](www.hisilicon.com/cn/products/smart-iot/ShortRangeWirelessIOT/Hi3861V100)）。


Besides, a HiSpark IPC DIY Camera ([Link](www.hihope.org/en/pro/pro1.aspx?mtt=23)) produced by HiHope is used as a Traffic Surveillance Camera you can see anywhere around the city. It uses a Hi3518 Camera SoC and a Hi3881 WiFi SoC, and more details and parameters about the SoC and Kit can be found on the link above and the website of HiSilicon ([link](www.hisilicon.com/en/products/smart-vision/consumer-camera/IOTVision/Hi3518EV300)).


此外，一组 HiHope 润和生产的 HiSpark IPC 摄像头套件（[链接](www.hihope.org/pro/pro1.aspx?mtt=23)）被用作模拟城市中随处可见的交通监控摄像头。它使用一片 Hi3518 芯片和一片 Hi3881 芯片，关于芯片和套件的更多细节和参数可以在上面的链接和海思的网站上找到（[链接](www.hisilicon.com/cn/products/smart-vision/consumer-camera/IOTVision/Hi3518EV300)）。

### Software

All chips except the IPC Camera have LiteOS installed, while OpenHarmony is installed on the IPC Camera.

除了 IPC 摄像头以外的所有芯片安装有 LiteOS 操作系统，而 IPC 摄像头安装了 OpenHarmony。

-----

More details will be provided soon, or you can create an issue to ask for further details. 

更多细节很快将被提供，或者你也可以创建一个问题以获得更多更进一步的细节。