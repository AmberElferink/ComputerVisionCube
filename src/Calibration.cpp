#include "Calibration.h"

#include <vector>
#include <iostream>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgcodecs.hpp>

#include "Texture.h"

/// Transform an OpenCV Perspective matrix into a OpenGL space
/// Projection matrix which is friendly to vertex shader transforms to
/// OpenGL normalized device coordinates (NDC) space and clip space for culling.
/// This removes the infinite far plane of OpenCV but points at infinity are
/// Mapped to behind the camera.
void fromCVPerspToGLProj(cv::Mat cvMat, mat4 &glMat) {
    double fx = cvMat.at<double>(0, 0);
    double fy = cvMat.at<double>(1, 1);
    double cx = cvMat.at<double>(0, 2);
    double cy = cvMat.at<double>(1, 2);

    const float zfar = 200.f;
    const float znear = 0.01f;

    // Infinite projection
    glMat[0] = -static_cast<float>(fx / cx);
    glMat[1] = 0.0f;
    glMat[2] = 0.0f;
    glMat[3] = 0.0f;

    glMat[4] = 0.0f;
    glMat[5] = static_cast<float>(fy / cy);
    glMat[6] = 0.0f;
    glMat[7] = 0.0f;

    glMat[8] = 0.0f;
    glMat[9] = 0.0f;
    glMat[10] = (zfar + znear) / (znear - zfar);
    glMat[11] = -1.0f;

    glMat[12] = 0.0f;
    glMat[13] = 0.0f;
    glMat[14] = 2.0f * zfar * znear / (znear - zfar);
    glMat[15] = 0.0f;
}

Calibration::Calibration(const cv::Size& patternSize, const cv::Size& cameraResolution, float sideSquare)
    : CameraMatKnown(false)
    , CameraMatrix(cv::Mat::eye(3, 3, CV_64F))
    // Identity matrix
    , ProjMat{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}
    , patternSize_(patternSize)
    , cameraResolution_(cameraResolution)
    , objectSpacePoints_(patternSize.width * patternSize.height)
    , DistortionCoefficients(cv::Mat::zeros(8, 1, CV_64F))
{
    initialObjectSpacetPoints_.reserve(10 * objectSpacePoints_.size());
    initialImageSpacePoints_.reserve(10 * objectSpacePoints_.size());
    for (int j = 0; j < patternSize_.height; j++)
    {
        for (int i = 0; i < patternSize_.width; i++)
        {
            // real world coordinates in meters of the inner corners if Z = 0 (so only x and y coordinates).
            objectSpacePoints_[i + j * patternSize_.width] = cv::Vec3f(i * sideSquare, j * sideSquare, 0);
        }
    }
}

void Calibration::LoadFromDirectory(const std::string &path)
{
    CalibImages.clear();
    CalibImageNames.clear();
    int calibFileCounter = 0;
    bool keepReading = true;
    while (keepReading) {
        std::string calibFileName = "calib" + std::to_string(calibFileCounter) + ".png";
        cv::Mat image = cv::imread(path + calibFileName);
        keepReading = image.data;
        if (keepReading) {
            if (DetectPattern(image, true))
            {
                std::cout << "Loaded " << calibFileName << std::endl;
                CalibImageNames.push_back(calibFileName);
                auto texture = Texture::create(image.cols, image.rows);
                if (texture) {
                    texture->upload(image);
                }
                CalibImages.emplace_back(std::move(texture));
            }
            else
            {
                std::cout << "No pattern found in " << calibFileName << std::endl;
            }
        }
        calibFileCounter++;
    }
    CalcCameraMat();
}

void Calibration::TakeCapture(const std::string &path, const cv::Mat& frame) {

    auto calibFileName = "calib" + std::to_string(CalibImages.size()) + ".png";
    if (!cv::imwrite(path + calibFileName, frame)) {
        std::cerr << "Failed to save " << path + calibFileName << std::endl;
    }
    CalibImageNames.push_back(calibFileName);
    auto texture = Texture::create(frame.cols, frame.rows);
    if (texture) {
        texture->upload(frame);
    }
    CalibImages.emplace_back(std::move(texture));
    std::cout << "Saved " << path + calibFileName << std::endl;

}

bool Calibration::DetectPattern(cv::Mat frame, bool addImage, bool drawCalibrationColors)
{
    bool chessBoardDetected = cv::findChessboardCorners(frame, patternSize_, imageSpacePoints_, cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE + cv::CALIB_CB_FAST_CHECK);

    if (chessBoardDetected && addImage) {
        initialObjectSpacetPoints_.push_back(objectSpacePoints_); // push back the default real world positions
        initialImageSpacePoints_.push_back(imageSpacePoints_); // push back detected corner positions in 2D image
    }
    if (drawCalibrationColors)
        cv::drawChessboardCorners(frame, patternSize_, imageSpacePoints_, chessBoardDetected);
    return chessBoardDetected;
}

bool Calibration::UpdateRotTransMat(mat4 &objectMatrix, float scaling_factor, bool usePrevFrame)
{
    if (CameraMatKnown) {
        if (!imageSpacePoints_.empty())
        {
            // calibrateCamera, when cameraMat and an approximation is already known
            try {
                cv::solvePnP(objectSpacePoints_, imageSpacePoints_, CameraMatrix, DistortionCoefficients, rotationVec_, translationVec_, usePrevFrame);
            } catch (cv::Exception& e) {
                return false;
            }

            cv::Mat rotation = cv::Mat::eye(3, 3, rotationVec_.type());
            cv::Rodrigues(rotationVec_, rotation);
            cv::Mat finalMatrix = scaling_factor * cv::Mat::eye(4, 4, rotation.type());
            finalMatrix.at<double>(3, 3) = 1.0;
            finalMatrix(cv::Range(0, 3), cv::Range(0, 3) ) *= rotation;
            finalMatrix(cv::Range(0, 3), cv::Range(3, 4) ) = translationVec_ * -1;

            finalMatrix = finalMatrix.t();

            for (int i = 0; i < 16; ++i)
            {
                objectMatrix[i] = finalMatrix.at<double>(i);
            }
            return true;
        }
    }
    return false;
}

void Calibration::CalcCameraMat()
{
    if (initialImageSpacePoints_.empty())
    {
        std::cerr << "make sure to capture calibration images first by loading "
                     "them from hard disk or capturing them\n";
        return;
    }
    cv::calibrateCamera(initialObjectSpacetPoints_, initialImageSpacePoints_, cameraResolution_, CameraMatrix, DistortionCoefficients, InitialRotationVectors, InitialTranslationVectors);
    fromCVPerspToGLProj(CameraMatrix, ProjMat);
    CameraMatKnown = true;
}
