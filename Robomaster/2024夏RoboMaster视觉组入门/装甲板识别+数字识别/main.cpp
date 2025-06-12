#include "Armor.h"

using namespace std;
using namespace cv;

int main() {
    string videoPath = "./target.mp4";
    string outputPath = "./final_example.avi";

    VideoCapture videoCapture;
    VideoWriter videoWriter;
    videoCapture.open(videoPath);
    // �����Ƶ�Ƿ�ɹ���
    if (!videoCapture.isOpened()) {
        std::cout << "Error opening video stream or file" << std::endl;
        exit(EXIT_FAILURE);
    }
    // ��ȡ��Ƶ��֡�ʺ�֡��С
    double fps = videoCapture.get(cv::CAP_PROP_FPS);
    int frame_width = static_cast<int>(videoCapture.get(cv::CAP_PROP_FRAME_WIDTH));
    int frame_height = static_cast<int>(videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT));
    Size frameSize(frame_width, frame_height);

    // ��ʼ�� VideoWriter
    videoWriter.open(outputPath, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, frameSize, true);

    // ��� VideoWriter �Ƿ�ɹ���ʼ��
    if (!videoWriter.isOpened()) {
        std::cout << "Could not open the output video file for write" << std::endl;
        exit(EXIT_FAILURE);
    }

    ArmorDetector processor;
    Color ENEMYCOLOR = RED;    // Enemy Color
    processor.setEnemyColor(ENEMYCOLOR);
    processor.loadSVM("./123svm.xml");
    processor.loadTemplate("./numbers/");
    
    Mat src;
    Mat final;
    while (1) {
        videoCapture >> src;
        if (src.empty()) break;
        final = processor.run(src);
        videoWriter.write(final);
        if (cv::waitKey(10) >= 0) break;
    }
    cv::destroyAllWindows();

    return 0;
}