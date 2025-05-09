from flask import Flask, request
import cv2
import numpy as np
import firebase_admin
from firebase_admin import credentials, storage, db
import io
import requests
from datetime import datetime

app = Flask(__name__)

# Konfigurasi Firebase
cred = credentials.Certificate("D:/SEMPRO/Bahan/PYTHON/python-server/firebase-adminsdk.json")
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://esp32-cam-97de9-default-rtdb.asia-southeast1.firebasedatabase.app',
    'storageBucket': 'esp32-cam-97de9.appspot.com'
    })

bucket = storage.bucket()

@app.route('/upload', methods=['POST'])
def upload_image():
    image_data = request.data
    np_image = np.frombuffer(image_data, np.uint8)
    image = cv2.imdecode(np_image, cv2.IMREAD_COLOR)

    # Convert the image to grayscale
    gray_image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    
    # Define the Region of Interest (ROI) for cropping
    # Set the coordinates as (x, y, width, height)
    x, y, w, h = 330, 100, 850, 1500  # Example coordinates
    roi = gray_image[y:y+h, x:x+w]
        
    # Apply bilateral filtering on the cropped image
    bilateral_image = cv2.bilateralFilter(roi, d=40, sigmaColor=75, sigmaSpace=75)
        
    # Apply CLAHE (Contrast Limited Adaptive Histogram Equalization)
    clahe = cv2.createCLAHE(clipLimit=2.0, tileGridSize=(8,8))
    clahe_image = clahe.apply(bilateral_image)
        
    # Apply Canny Edge Detection on the filtered image
    edges = cv2.Canny(clahe_image, 20, 2)  # Example thresholds
    
    # Ambil gambar master dari Firebase untuk perbandingan
    blob = bucket.blob('Master_Image_hilmy_KR_07d0d933-ecd5-4aff-9359-3af55cb0d59f.jpg')
    master_image_data = blob.download_as_bytes()
    master_img = cv2.imdecode(np.frombuffer(master_image_data, np.uint8), cv2.IMREAD_GRAYSCALE)

    # Perbandingan menggunakan Line Segment Detection (LSD)
    lsd = cv2.createLineSegmentDetector(0)
    lines1 = lsd.detect(edges)[0]
    lines2 = lsd.detect(master_img)[0]

    if lines1 is not None and lines2 is not None:
        similarity = compare_lines(lines1, lines2)
        result = "Identik" if similarity >= 0.9 else "Tidak Identik"
    else:
        result = "Gagal"


    #Simpan gambar ke firebase menggunakan format timestamp
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    image_name = f"Processed_Image_{result}_{timestamp}.jpg"  # Nama file berdasarkan hasil dan timestamp
    cv2.imwrite(image_name, edges)

    comp_path = f"comparison_images/{image_name}"
    blob = bucket.blob(comp_path)  # Folder khusus untuk gambar

    # Mengunggah gambar
    blob.upload_from_filename(image_name)
    print(f"Gambar hasil disimpan di Firebase Storage dengan nama: {image_name}")

    # Dapatkan URL publik untuk gambar yang diunggah
    blob.make_public()
    image_url = blob.public_url
    
    send_result_to_wemos(result)
    save_result_to_realtime_database(image_url, result)

    # Return the result to client
    return result, 200 if result != "Gagal" else 500
    
def compare_lines(lines1, lines2):
    """
    Menghitung kesamaan antara dua set garis berdasarkan jumlah dan orientasinya.
    :param lines1: Array garis pertama yang terdeteksi
    :param lines2: Array garis kedua yang terdeteksi
    :return: Nilai kesamaan (0 hingga 1)
    """
    if lines1 is None or lines2 is None:
        return 0.0

    # Menghitung jumlah garis di masing-masing gambar
    num_lines1 = len(lines1)
    num_lines2 = len(lines2)

    # Perbandingan jumlah garis
    count_similarity = min(num_lines1, num_lines2) / max(num_lines1, num_lines2)

    # Ekstraksi orientasi garis
    angles1 = [np.arctan2((l[0][3] - l[0][1]), (l[0][2] - l[0][0])) for l in lines1]
    angles2 = [np.arctan2((l[0][3] - l[0][1]), (l[0][2] - l[0][0])) for l in lines2]

    # Konversi orientasi ke rentang [0, pi]
    angles1 = [angle if angle >= 0 else angle + np.pi for angle in angles1]
    angles2 = [angle if angle >= 0 else angle + np.pi for angle in angles2]

    # Perbandingan orientasi garis
    hist1, _ = np.histogram(angles1, bins=36, range=(0, np.pi))
    hist2, _ = np.histogram(angles2, bins=36, range=(0, np.pi))

    # Normalisasi histogram
    hist1 = hist1 / np.linalg.norm(hist1) if np.linalg.norm(hist1) > 0 else hist1
    hist2 = hist2 / np.linalg.norm(hist2) if np.linalg.norm(hist2) > 0 else hist2

    # Menghitung kesamaan orientasi menggunakan cosine similarity
    orientation_similarity = np.dot(hist1, hist2)

    # Menggabungkan kesamaan jumlah dan orientasi (rata-rata)
    total_similarity = (count_similarity + orientation_similarity) / 2

    return total_similarity

def send_result_to_wemos(result):
    wemos_url = 'http://192.168.251.40:8000/result'  # Ganti dengan alamat IP Wemos Lolin32
    response = requests.post(wemos_url, data=result)
    if response.status_code == 200:
        print("Hasil perbandingan berhasil dikirim ke WEMOS: ", result)
    else:
        print("Gagal mengirim hasil:", response.status_code)
    return response.status_code

def save_result_to_realtime_database(image_url, result):
    # Menyimpan hasil perbandingan ke Realtime Database
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    ref = db.reference("comparison_results_test_NR/KR/")
    result_data = ({
        "timestamp": timestamp,
        "result": result,
        "url": image_url
    })
    ref.push(result_data)  # Folder di Realtime Database
    print(f"Hasil perbandingan disimpan di Realtime Database dengan Hasil: {result}")
    return result_data

def save_image_to_firebase_storage(edges, result):
    #Simpan gambar ke firebase menggunakan format timestamp
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    image_name = f"Processed_Image_{result}_{timestamp}.jpg"  # Nama file berdasarkan hasil dan timestamp
    cv2.imwrite(image_name, edges)

    comp_path = f"comparison_image/test/{image_name}"
    blob = bucket.blob(comp_path)  # Folder khusus untuk gambar

    # Mengunggah gambar
    blob.upload_from_filename(image_name)
    print(f"Gambar hasil disimpan di Firebase Storage dengan nama: {image_name}")

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8000, debug=True)
