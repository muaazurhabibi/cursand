import cv2
import mediapipe
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
from pynput import keyboard
import threading
import struct
from win32api import GetSystemMetrics

from ipc import ipc

SCREEN_DIMS:tuple[int, int] = (GetSystemMetrics(0), GetSystemMetrics(1))

'''
    lines: lines drawn between landmarks, 21 for left, 21 for right
    circles: landmark positions of hand, 21 for left, 21 for right
'''
def draw_landmarks_on_image(rgb_image, detection_result):
    annotated_image = rgb_image.copy()

    hand_landmarks_list = detection_result.hand_landmarks

    # Connections between the 21 hand landmarks
    HAND_CONNECTIONS = [
        # Thumb
        (0, 1),
        (1, 2),
        (2, 3),
        (3, 4),

        # Index finger
        (0, 5),
        (5, 6),
        (6, 7),
        (7, 8),

        # Middle finger
        (5, 9),
        (9, 10),
        (10, 11),
        (11, 12),

        # Ring finger
        (9, 13),
        (13, 14),
        (14, 15),
        (15, 16),

        # Pinky
        (13, 17),
        (17, 18),
        (18, 19),
        (19, 20),

        # Palm
        (0, 17),
    ]

    all_points = []
    lines_coords = []
    circles_coords = []

    for hand_landmarks in hand_landmarks_list:

        points = []

        # Convert normalized coordinates → pixel coordinates: WRONG
        # We convert it in C program to be relative to our SCREEN not webcam: ALSO WRONG
        # We'll just get the screen dims here FOR NOW
        for landmark in hand_landmarks:
            x = int(landmark.x * SCREEN_DIMS[0])
            y = int(landmark.y * SCREEN_DIMS[1])
            #x = float(landmark.x)
            #y = float(landmark.y)
            points.append((x, y))

        all_points.append(points)

        # Draw connections
        for start_index, end_index in HAND_CONNECTIONS:
            start = points[start_index]
            end = points[end_index]

            lines_coords.append(
                (start, end)
            )

            cv2.line(
                annotated_image,
                start,
                end,
                (0, 255, 0),
                2
            )

        # Draw landmarks
        for x, y in points:
            circles_coords.append((x, y))
            cv2.circle(
                annotated_image,
                (x, y),
                5,
                (255, 0, 0),
                -1
            )

    return lines_coords, circles_coords

q_pressed:threading.Event = threading.Event()
def on_press(key):
    global q_pressed

    try:
        if key.char == "q":
            q_pressed.set()
            return False
    except AttributeError:
        pass

if __name__ == "__main__":
    cam = cv2.VideoCapture(0)
    base_options = python.BaseOptions(model_asset_path="mp_tasks/hand_landmarker.task")
    options = vision.HandLandmarkerOptions(base_options=base_options, num_hands=2)
    detector = vision.HandLandmarker.create_from_options(options)
    shm, semRead, semWrite = ipc.shm_init()
    listener = keyboard.Listener(on_press=on_press)
    listener.start()

    while True:
        success, bgr_frame = cam.read()
        if not success:
            break

        frame = cv2.flip(cv2.cvtColor(bgr_frame, cv2.COLOR_BGR2RGB), 1)
        res = detector.detect(mediapipe.Image(image_format=mediapipe.ImageFormat.SRGB, data=frame))

        lines, circles = draw_landmarks_on_image(frame, res)
        data:bytes = b''

        for (x1, y1), (x2, y2) in lines:
            data += struct.pack("=ffff", x1, y1, x2, y2)
        for _ in range(42 - len(lines)):
            data += struct.pack("=ffff", -1.0, -1.0, -1.0, -1.0)

        for x, y in circles:
            data += struct.pack("=ff", x, y)
        for _ in range(42 - len(circles)):
            data += struct.pack("=ff", -1.0, -1.0)

        if len(data) > 0 and not all(val == -1.0 for val in struct.unpack(f"={len(data) // 4}f", data)):
            if ipc.shm_write(shm, data, semRead, semWrite):
                print("Data sent successfully")

        #cv2.imshow("hewwo", bgr_frame)
        cv2.waitKey(1)
        if q_pressed.is_set():
            break

    cam.release()
    cv2.destroyAllWindows()