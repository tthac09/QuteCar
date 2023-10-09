from ultralytics import YOLO
import torch

data_path = "RedCross.v1i.yolov8/data.yaml"
device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")

# 加载模型
model = YOLO("yolov8n.pt")

# Use the model
results = model.train(data=data_path, save=False, epochs=64, batch=8)  # 训练模型
