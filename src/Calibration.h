#include <memory>
#include <vector>
#include <string>
#include <opencv2/core/mat.hpp>

// mat4 is equivalent to float[16]
typedef float mat4[16];
class Texture;

/// python code adapted and translated:
/// https://opencv-python-tutroals.readthedocs.io/en/latest/py_tutorials/py_calib3d/py_calibration/py_calibration.html
/// combined with parts of cpp code:
/// https://docs.opencv.org/2.4/doc/tutorials/calib3d/camera_calibration/camera_calibration.html
class Calibration {
  public:
    bool CameraMatKnown;
    cv::Mat CameraMatrix; // empty until calibration is complete
    mat4 CameraProjMat;

  /// list of all vec3s, one for each calibrated image. Not updated after that.
  cv::Mat InitialTranslationVectors;
  /// list of all vec3s, one for each calibrated image. Not updated after that
  cv::Mat InitialRotationVectors;

  cv::Mat DistortionCoefficients;

  std::vector<std::string> CalibImageNames;
  std::vector<std::unique_ptr<Texture>> CalibImages;

  private:
    cv::Size cameraResolution_;
    cv::Size patternSize_;
    // these values are for the extrinsics, so they only contain info on the
    // current frame.
    cv::Mat rotationVec_;
    cv::Mat translationVec_;
    std::vector<cv::Point2f> imageSpacePoints_;
    /// 3d points in real world space with z = 0 (2D paper)
    std::vector<cv::Vec3f> objectSpacePoints_;
    // these are lists for each calibration image, only used at the start for
    // getting the cameraMat outer vector: for each frame, inner vector: every
    // corner point, Vec: known coordinates
    /// every frame captures the same 3d real world points, so the objectSpacePoints_ vector over and over
    std::vector<std::vector<cv::Vec3f>> initialObjectSpacetPoints_;
    /// in each frame the capture is different, so different image coordinates for each frame
    std::vector<std::vector<cv::Point2f>> initialImageSpacePoints_;

public:
    /// calibration with chessboard pattern. PatternWidth and height are the inner corners (squares - 1)
    Calibration(const cv::Size& patternSize, const cv::Size& cameraResolution, float sideSquare);
    void LoadFromDirectory(const std::string& path);
    bool DetectPattern(cv::Mat frame, bool addImage, bool drawCalibrationColors = true);
    /// update the rotation mat. Returns true if correctly updated.
    bool UpdateRotTransMat(mat4& rotTransMat, bool usePrevFrame);
    /// get the camera matrix via opencv and copy it to a float16 mat4.
    /// automatically also updates rotation and translation vectors
    void CalcCameraMat();
};
