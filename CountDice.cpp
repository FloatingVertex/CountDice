#include <iostream>
#include <opencv2/opencv.hpp>

/**
 * Takes a vector of contours and returns only the subset with an area greater than min area and less than max area
 * @param allContours vector of contours to filter
 * @param minArea     minArea (exclusive)
 * @param maxArea     maxArea (exclusive)
 * @return vector of contours that were in the specified area range
 */
std::vector<std::vector<cv::Point>> FilterContoursByArea(
        std::vector<std::vector<cv::Point>> allContours,
        const double minArea,
        const double maxArea)
{
    std::vector<std::vector<cv::Point>> filtered;
    for(const auto& contour : allContours)
    {
        double area = cv::contourArea(contour);
        if(area < maxArea && area > minArea)
        {
            filtered.push_back(contour);
        }
    }
    return filtered;
}

int main( int argc, char** argv ) {
    if( argc != 3)
    {
        std::cout <<" Usage: CountDice.exe ImageToProcessPath OutputImagePath" << std::endl;
        return -1;
    }

    cv::Mat colorImage;
    colorImage = imread(argv[1], cv::IMREAD_COLOR); // Read the file

    if(!colorImage.data )
    {
        std::cout << "Could not open the image" << std::endl ;
        return -1;
    }

    // Threshold image, with the Dice as white and the dots and background are black
    cv::Mat threholdedImage;
    cv::cvtColor(colorImage, threholdedImage, CV_BGR2GRAY);// convert to grayscale for the threshold operation

    // Use static threshold determined through experimentation because otsu's method has problems when
    // there are no dice on the table
    static const double thresh = 160;
    static const double maxValue = 255;
    cv::threshold(threholdedImage,threholdedImage, thresh, maxValue, cv::THRESH_BINARY);

    // find all contours
    std::vector<std::vector<cv::Point> > contours;
    cv::Mat contourOutput = threholdedImage.clone();
    cv::findContours( contourOutput, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE );

    // Assume dice are about the same size in pixels (~11000) in each image
    auto diceContours = FilterContoursByArea(contours, 5000, 30000);
    // Assume dots are about the same size in pixels (~400) in each image
    // The area filter gets rid of dots on the side of dice that are still visible and small noise contours
    auto dotContours = FilterContoursByArea(contours, 200, 1000);

    // Draw dot counts next to each die
    cv::Scalar textColor{0, 255, 0};
    int totalsDots = 0;
    for(const auto& diceContour: diceContours)
    {
        int dotsCount = 0;
        // Find the number of dots in this die
        for(const auto& dotContour: dotContours)
        {
            // Contours don't intersect so if one point of a dot contour is inside a dice contour,
            // it is entirely in the dice contour
            if(cv::pointPolygonTest(diceContour,dotContour[0],false) > 0){
                dotsCount++;
            }
        }
        auto boundingBox = cv::boundingRect(diceContour);
        cv::Point textPosition{boundingBox.x + boundingBox.width, boundingBox.y + boundingBox.height};
        cv::putText(colorImage, std::to_string(dotsCount), textPosition, cv::FONT_HERSHEY_SIMPLEX, 1, textColor);

        totalsDots += dotsCount;
    }
    // Add text for total count
    cv::putText(colorImage, "Sum " + std::to_string(totalsDots), cv::Point(10,35), cv::FONT_HERSHEY_SIMPLEX, 1.5, textColor);

    // Draw the dot and dice contours on original image
    const cv::Scalar diceColor{0, 255, 0};
    cv::drawContours(colorImage, diceContours, -1, diceColor, 3);
    const cv::Scalar dotsColor{255, 0, 0};
    cv::drawContours(colorImage, dotContours, -1, dotsColor, 3);

    // Save final Image
    imwrite( argv[2] , colorImage );

    // Display final Image
    namedWindow( "Labeled Image", cv::WINDOW_AUTOSIZE );
    imshow( "Labeled Image", colorImage );
    cv::waitKey(0);

    return 0;
}