#include <cstddef>

#include <opencv2/opencv.hpp>

#include <yaml-cpp/yaml.h>

#include <filesystem>

#include <iostream>

namespace fs = std::filesystem;

void detectAndDisplay(cv::Mat frame);

cv::CascadeClassifier face_cascade;
cv::CascadeClassifier eye_cascade;

int main() {
  YAML::Node config = YAML::LoadFile(NOIR_SETTINGS_PATH);

  fs::path face_cascade_path =
      NOIR_DATA_PATH / (fs::path)config["face_cascade"].as<std::string>();
  fs::path eye_cascade_path =
      NOIR_DATA_PATH / (fs::path)config["eye_cascade"].as<std::string>();

  if (!face_cascade.load(face_cascade_path)) {
    std::cout << "--(!)Error loading face cascade\n";
    return -1;
  }

  if (!eye_cascade.load(eye_cascade_path)) {
    std::cout << "--(!)Error loading eye cascade\n";
    return -1;
  }

  int camera_device = config["camera"].as<int>();

  cv::VideoCapture capture;
  capture.open(camera_device);

  if (!capture.isOpened()) {
    std::cout << "--(!)Error opeing video capture\n";
    return -1;
  }

  cv::Mat frame;

  while (capture.read(frame)) {
    if (frame.empty()) {
      std::cout << "--(!) No captured frame -- break!\n";
      break;
    }
    detectAndDisplay(frame);

    if (cv::waitKey(10) == 27) {
      break;
    }
  }

  return 0;
}

void detectAndDisplay(cv::Mat frame) {
  cv::Mat frame_gray;
  cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
  cv::equalizeHist(frame_gray, frame_gray);

  // -- Detect faces
  std::vector<cv::Rect> faces;
  face_cascade.detectMultiScale(frame_gray, faces);

  for (const auto &face : faces) {
    // defining ellipse around face
    cv::Point center(face.x + face.width / 2, face.y + face.height / 2);
    cv::ellipse(frame, center, cv::Size(face.width / 2, face.height / 2), 0, 0,
                360, cv::Scalar(255, 0, 255), 4);
    // reducing search area to face area would be my guess                
    cv::Mat faceROI = frame_gray(face);

    //-- In each face, detect eyes
    std::vector<cv::Rect> eyes;
    eye_cascade.detectMultiScale(faceROI, eyes);
    // defining circles around eyess
    for (const auto &eye : eyes) {
      cv::Point eye_center(face.x + eye.x + eye.width / 2,
                           face.y + eye.y + eye.height / 2);
      int radius = cvRound((eye.width + eye.height) * 0.25);
      cv::circle(frame, eye_center, radius, cv::Scalar(255, 0, 0), 4);
    }
  }
  cv::imshow("Capture - Face detection", frame);
}