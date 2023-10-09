import cv2
from ultralytics import YOLO
import torch
from car_action import procMQTTMsg1,procMQTTMsg2

# Load the YOLOv8 model
device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
weight_path = "D:/Engineering_Innovation_Practice/car_detect_p/best.pt"                                        # FIXME 修改模型权重地址
source = 1                                                      # FIXME camera path
classes = ["RedCross"]

model = YOLO(weight_path)

# Open the video file
cap = cv2.VideoCapture(source)

# Loop through the video frames
while cap.isOpened():
    # Read a frame from the video
    success, frame = cap.read()

    if success:
        # Run YOLOv8 inference on the frame
        results = model(source=frame, show=True)

        cls = results[0].boxes.cls.tolist()
        if len(cls) != 0:
            idx = int(cls[0])
#            procMQTTMsg1(classes[idx]) # 避让
            procMQTTMsg2(classes[idx]) # 路口
            break

        if cv2.waitKey(1) & 0xFF == ord("q"):
            break
    else:
        # Break the loop if the end of the video is reached
        break

# Release the video capture object and close the display window
cap.release()
cv2.destroyAllWindows()
