#include"Armor.h"

using namespace cv;
using namespace ml;
using namespace std;

ArmorNumClassifier::ArmorNumClassifier() {
	svm = ml::SVM::create();
	armorImgSize = Size(40, 40);
	p = Mat();

	warpPerspective_mat = Mat(3, 3, CV_32FC1);
	dstPoints[0] = Point2f(0, 0);
	dstPoints[1] = Point2f(armorImgSize.width, 0);
	dstPoints[2] = Point2f(armorImgSize.width, armorImgSize.height);
	dstPoints[3] = Point2f(0, armorImgSize.height);
}

ArmorNumClassifier::~ArmorNumClassifier() {}

void ArmorNumClassifier::loadSvmModel(const char* model_path, Size armorImgSize) {
	svm = StatModel::load<SVM>(model_path);
	if (svm.empty())
	{
		cout << "Svm load error! Please check the path!" << endl;
		exit(0);
	}
	this->armorImgSize = armorImgSize;

	//set dstPoints (the same to armorImgSize, as it can avoid resize armorImg)
	dstPoints[0] = Point2f(0, 0);
	dstPoints[1] = Point2f(armorImgSize.width, 0);
	dstPoints[2] = Point2f(armorImgSize.width, armorImgSize.height);
	dstPoints[3] = Point2f(0, armorImgSize.height);
}

void ArmorNumClassifier::loadImg(Mat& srcImg) {

	//copy srcImg as warpPerspective_src
	(srcImg).copyTo(warpPerspective_src);

	//preprocess srcImg for the goal of acceleration
	
	cvtColor(warpPerspective_src, warpPerspective_src, 6);  //CV_BGR2GRAY=6
	threshold(warpPerspective_src, warpPerspective_src, 3.5, 255, THRESH_BINARY);
}

void ArmorNumClassifier::getArmorImg(ArmorBox& armor) {
	//set the armor vertex as srcPoints
	for (int i = 0; i < 4; i++)
		srcPoints[i] = armor.armorVertices[i];

	//get the armor image using warpPerspective
	warpPerspective_mat = getPerspectiveTransform(srcPoints, dstPoints);  // 透射变换矩阵
	warpPerspective(warpPerspective_src, warpPerspective_dst, warpPerspective_mat, armorImgSize, INTER_NEAREST, BORDER_CONSTANT, Scalar(0)); //warpPerspective to get armorImage
	warpPerspective_dst.copyTo(armor.armorImg); //copyto armorImg
	imshow("pofang", armor.armorImg);
}

void ArmorNumClassifier::setArmorNum(ArmorBox& armor) {

	// adapt armorImg to the SVM model sample-size requirement
	p = armor.armorImg.reshape(1, 1);
	p.convertTo(p, CV_32FC1);

	//set armor number according to the result of SVM  
	armor.armorNum = (int)svm->predict(p);
}

void ArmorNumClassifier::loadTemplates(const string& filename) {
	for (int i = 0; i <= 9; i++) {
		string finalname = filename + to_string(i) + ".jpg";
		Mat templateImg = imread(finalname, IMREAD_GRAYSCALE);
		if (templateImg.empty()) {
			cout << "could not open templates" << endl;
			exit(EXIT_FAILURE);
		}
		templates.push_back(templateImg);
	}
}

void ArmorNumClassifier::setNumber(ArmorBox& armor) {

	if (armor.armorImg.empty()) {
		cout << "ROI is empty!" <<endl;
		return;
	}

	double highestCorrelation = 0;
	int bestMatchIndex = -1;

	for (size_t i = 0; i < templates.size(); ++i) {
		Mat resizedTemplate = templates[i];
		Size templateSize = resizedTemplate.size();
		Size roiSize = armor.armorImg.size();

		// 检查模板图像是否大于目标图像，如果是则缩小模板图像
		if (templateSize.width > roiSize.width || templateSize.height > roiSize.height) {
			double scale = min(static_cast<double>(roiSize.width) / templateSize.width,
				static_cast<double>(roiSize.height) / templateSize.height);
			Size newSize(static_cast<int>(templateSize.width * scale), static_cast<int>(templateSize.height * scale));
			resize(resizedTemplate, resizedTemplate, newSize);
		}

		Mat result;
		matchTemplate(armor.armorImg, resizedTemplate, result, TM_CCOEFF_NORMED);
		double maxVal;
		minMaxLoc(result, nullptr, &maxVal, nullptr, nullptr);

		if (maxVal > 0.375 && (bestMatchIndex == -1 || maxVal > highestCorrelation)) {
			bestMatchIndex = i;
			highestCorrelation = maxVal;
		}
	}

	armor.armorNum = bestMatchIndex;
}