import pygame.camera
pygame.camera.init()
camera_id_list = pygame.camera.list_cameras()
print(camera_id_list)
