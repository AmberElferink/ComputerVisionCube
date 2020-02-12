#include <vector>
#include <opencv2/calib3d.hpp>

// mat4 is equivalent to float[16]
typedef float mat4[16];

//copies Mat4 from opencv to opengl (float[16]) mat
void fromCVMatToGLMat(cv::Mat cvMat, mat4& glMat)
{
    //convert mat to correct type (float 1 channel)
    if (cvMat.type() != CV_32FC1) {
        int type = cvMat.type();
        cvMat.convertTo(cvMat, CV_32FC1);
    }
    if (cvMat.cols == 3 && cvMat.rows == 3)
    {
        //appends a row and column to make the matrix 4x4. Example:
        //from:
        // [732,   0, 309;
        //    0, 724,  84;
        //    0,   0,   1]
        // to:
        // [732, 0, 309, 0;
         //  0, 724, 84, 0;
         //  0,   0,  1, 0;
         //  0,   0,  0, 1]

        //extend to 4 cols and rows
        cv::Mat row = cv::Mat::zeros(1, 3, cvMat.type());
        cv::Mat col = cv::Mat::zeros(4, 1, cvMat.type());
        cvMat.push_back(row);
        cv::hconcat(cvMat, col, cvMat);
        cvMat.at<float>(3, 3) = 1;
    }

    //https://stackoverflow.com/questions/44409443/how-a-cvmat-translate-from-to-a-glmmat4
    else if (cvMat.cols != 4 || cvMat.rows != 4 || cvMat.type() != CV_32FC1)
    {
        std::cout << "Matrix conversion error!" << std::endl;
        return;
    }
    // cv::transpose(cvMat, cvMat);
    memcpy(glMat, cvMat.data, 16 * sizeof(float));
}

//python code adapted and translated: https://opencv-python-tutroals.readthedocs.io/en/latest/py_tutorials/py_calib3d/py_calibration/py_calibration.html
//combined with parts of cpp code: https://docs.opencv.org/2.4/doc/tutorials/calib3d/camera_calibration/camera_calibration.html
class Calibration
{
  public:
    cv::Mat cameraMatrix; //empty until calibration is complete
    cv::Mat dist_coeffs;
    cv::Mat translation_vectors;
    cv::Mat rotation_vectors;

    bool cameraMatKnown = false;

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
            std::string calibFileName = path + "calib" + std::to_string(calibFileCounter) + ".png";
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
        {
            objectPoints.push_back(objp); //push back the default real world positions
            imgPoints.push_back(corners); //push back detected corner positions in 2D image

            printf("calibration image added\n");
        }
        if (drawCalibrationColors)
			cv::drawChessboardCorners(frame, patternSize, corners, chessBoardDetected);
    }

    void UpdateRotTransMat(cv::Size cameraSize, mat4& rotTransMat)
    {
        //if (cameraMatKnown)
        //    //calibrateCamera, when cameraMat is already known
        //    cv::solvePnP(objectPoints, imgPoints, cameraMatrix, dist_coeffs, rotation_vectors, translation_vectors, true, )
    }

    //get the camera matrix via opencv and copy it to a float16 mat4.
    //automatically also updates rotation and translation vectors
    void CalcCameraMat(cv::Size cameraSize, mat4& cameraMat)
    {
        cv::calibrateCamera(objectPoints, imgPoints, cameraSize, cameraMatrix, dist_coeffs, rotation_vectors, translation_vectors);
        fromCVMatToGLMat(cameraMatrix, cameraMat);
        cameraMatKnown = true;

        std::cout << "printing opengl cameramat result:\n";
        for(int i = 0; i < 16; i++)
        {
            std::cout << cameraMat[i] << ", ";
        }
        std::cout << "\n";
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
