#include <vector>
#include <opencv2/calib3d.hpp>


//python code adapted and translated: https://opencv-python-tutroals.readthedocs.io/en/latest/py_tutorials/py_calib3d/py_calibration/py_calibration.html
//combined with parts of cpp code: https://docs.opencv.org/2.4/doc/tutorials/calib3d/camera_calibration/camera_calibration.html
class Calibration
{
  public:
    cv::Mat cameraMatrix; //empty until calibration is complete
    cv::Mat dist_coeffs;
    cv::Mat translation_vectors;
    cv::Mat rotation_vectors;

private:
    int mmSideSquare = 23;
    cv::Size patternSize; //= cv::Size(6, 9);
    std::vector<cv::Vec3f> objp;  //3d points in real world space with z = 0 (2D paper)

    //outer vector: for each frame, inner vector: every corner point, Vec: known coordinates
    std::vector<std::vector<cv::Vec3f>> objectPoints; //every frame captures the same 3d real world points, so the objp vector over and over
    std::vector<std::vector<cv::Point2f>> imgPoints; //in each frame the capture is different, so different image coordinates for each frame


public:
    //calibration with checkboard pattern. PatternWidth and height are the inner corners (squares - 1)
    Calibration(const int patternWidth, const int patternHeight, const int mmSideSquare)
	{
        cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
        //cameraMatrix.at<double>(0,0) = 1.0; //if using fixed aspec ratio (cpp code link)
        dist_coeffs = cv::Mat::zeros(8, 1, CV_64F);

        patternSize = cv::Size(patternWidth, patternHeight);
        int cornerNr = patternSize.width * patternSize.height;
        objp.resize(cornerNr);
	
        objectPoints.reserve(10 * objp.size());
        imgPoints.reserve(10 * cornerNr);

		for (int i = 0; i < patternSize.width; i++) 
		{
			for (int j = 0; j < patternSize.height; j++) 
			{ 
				// real world coordinates in mm of the inner corners if Z = 0 (so only x and y coordinates).
                objp[i + j * patternSize.width] = cv::Vec3f(i * mmSideSquare, j * mmSideSquare, 0);
            }
        }
	}

	void LoadFromSaved(std::string path)
    {

        int calibFileCounter = 0;
       bool keepReading = true;
        while(keepReading)
        {
            std::string calibFileName = path + std::to_string(calibFileCounter) + ".png";
            calibFileCounter++;
            cv::Mat image = cv::imread(calibFileName);
            keepReading = image.data;
            if(keepReading)
                DetectPattern(image, true);
        }
    }

	void DetectPattern(cv::Mat& frame, const bool addImage, const bool drawCalibrationColors = true)
    {
        std::vector<cv::Point2f> corners;
        bool chessBoardDetected = cv::findChessboardCorners(frame, patternSize, corners);
        if (chessBoardDetected && addImage)
        if (chessBoardDetected && addImage)
        {
            objectPoints.push_back(objp); //push back the default real world positions
            imgPoints.push_back(corners); //push back detected corner positions in 2D image

            printf("calibration image added\n");
        }
        if (drawCalibrationColors)
			cv::drawChessboardCorners(frame, patternSize, corners, chessBoardDetected);
    }

    void CalcCameraMat(cv::Size cameraSize)
    {
        cv::calibrateCamera(objectPoints, imgPoints, cameraSize, cameraMatrix, dist_coeffs, rotation_vectors, translation_vectors);
    }

    void PrintResults() {
        std::cout << "camera matrix:\n";
        std::cout << cameraMatrix << "\n";
        std::cout << "rotation vec:\n";
        std::cout << rotation_vectors << "\n";
        std::cout << "translation vec:\n";
        std::cout << translation_vectors << "\n";
        std::cout << "distance coeffs\n";
        std::cout << dist_coeffs << "\n";
    }

};