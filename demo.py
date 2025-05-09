from flask import Flask, request
import firebase_admin
from firebase_admin import credentials, storage
import os
import cv2
import numpy as np
import uuid

app = Flask(__name__)

# Konfigurasi Firebase
cred = credentials.Certificate("D:/SEMPRO/Bahan/PYTHON/python-server/firebase-adminsdk.json")
firebase_admin.initialize_app(cred, {'storageBucket': 'esp32-cam-97de9.appspot.com'})

bucket = storage.bucket()

@app.route('/upload', methods=['POST'])
def upload_image():
    if request.method == 'POST':
        image_data = request.data  # Ambil data gambar
        # Konversi data gambar ke format yang bisa diproses OpenCV
        np_array = np.frombuffer(image_data, np.uint8)
        image = cv2.imdecode(np_array, cv2.IMREAD_COLOR)

        # Load image
        #image = cv2.imread('received_image.jpg')

        # Convert the image to grayscale
        gray_image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

        # Display the grayscale image
        #cv2.imshow('Grayscale Image', gray_image)

        # Define the Region of Interest (ROI) for cropping
        # Set the coordinates as (x, y, width, height)
        x, y, w, h = 330, 100, 850, 1500  # Example coordinates
        roi = gray_image[y:y+h, x:x+w]

        # Display the cropped image
        #cv2.imshow('Cropped Image (ROI)', roi)

        # Apply bilateral filtering on the cropped image
        bilateral_image = cv2.bilateralFilter(roi, d=40, sigmaColor=75, sigmaSpace=75)

        # Display the filtered image
        #cv2.imshow('Median Image', median_image)

        # Apply CLAHE (Contrast Limited Adaptive Histogram Equalization)
        clahe = cv2.createCLAHE(clipLimit=2.0, tileGridSize=(8,8))
        clahe_image = clahe.apply(bilateral_image)

        # Display the CLAHE result
        cv2.imshow('CLAHE Image', clahe_image)

        # Apply Canny Edge Detection on the filtered image
        edges = cv2.Canny(clahe_image, 20, 2)  # Example thresholds

        # Display the Canny edge detection result
        cv2.imshow('Canny Edge Detection', edges)

        # Apply Line Segment Detector (LSD) to detect lines
        # Initialize LSD
        lsd = cv2.createLineSegmentDetector(0)

        # Detect lines in the edge image
        lines = lsd.detect(edges)[0]  # Posisi 0 dari tuple yang dikembalikan adalah garis yang terdeteksi

        # Draw the detected lines on the image
        lsd_image = cv2.cvtColor(edges, cv2.COLOR_GRAY2BGR)  # Konversi ke BGR untuk garis berwarna
        if lines is not None:
            for line in lines:
                x0, y0, x1, y1 = map(int, line[0])

                # Draw lines with desired color (contoh: merah)
                cv2.line(lsd_image, (x0, y0), (x1, y1), (0, 0, 255), 2)

                # Display the Line Segment Detector result
                cv2.imshow('Line Segment Detector', lsd_image)
                
        processed_img_path = f'Master_Image_Test6_KN_{uuid.uuid4()}.jpg'
        cv2.imwrite(processed_img_path, edges)

        new_path=f"folder_baru/{processed_img_path}"

        # Simpan gambar di Firebase Storage
        blob = bucket.blob(new_path)
        blob.upload_from_filename(processed_img_path)

        # Hapus file lokal setelah diupload
        os.remove(processed_img_path)

        return "Image uploaded and processed successfully", 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8000, debug=True)

# Wait for any key to close the windows
#cv2.waitKey(0)
#cv2.destroyAllWindows()

# Optionally, save the processed images
#cv2.imwrite('gray_image.jpg', gray_image)
#cv2.imwrite('clahe_image.jpg', clahe_image)
#cv2.imwrite('cropped_image.jpg', roi)
#cv2.imwrite('filtered_image.jpg', filtered_image)
#cv2.imwrite('canny_edges.jpg', edges)
#cv2.imwrite('lsd_lines.jpg', lsd_image)
