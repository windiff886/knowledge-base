#include"Armor.h"
using namespace cv;
using namespace std;

/**
* @brief:  检测所有可能的灯条
*/
void ArmorDetector::findLights() {
	vector<vector<Point>> lightContours;  //候选灯条轮廓
	Mat contourImg; //给findContours用的图像，防止findContours改变roiImg
	srcImg_binary.copyTo(contourImg); //a copy of roiImg, contourImg
	findContours(contourImg, lightContours, 0, 2); //CV_RETR_EXTERNAL = 0, CV_CHAIN_APPROX_SIMPLE = 2       最耗时的操作，优化方向
	RotatedRect lightRect;  // 拟合椭圆来的灯条旋转矩形
	LightBar light;  //临时灯条
	for (const auto& lightContour : lightContours) {
		if (lightContour.size() < 6) continue; // 轮廓点数小于6，不可拟合椭圆
		if (contourArea(lightContour) < armorParam.min_area) continue; //面积筛选滤去小发光点

		lightRect = fitEllipse(lightContour); //拟合椭圆
		light = LightBar(lightRect);//构造为灯条

		if (abs(light.angle) > armorParam.max_angle) continue; //角度筛选，滤去一些竖直偏角偏大的

		lights.emplace_back(light);
	}
	if (lights.size() < 2) {
		state = LIGHTS_NOT_FOUND; //灯条少于两条则设置状态为没找到灯条
		return; 
	}

	// 将灯条从左到右排序
	sort(lights.begin(), lights.end(),
		[](LightBar& a1, LightBar& a2) {
			return a1.center.x < a2.center.x; });
	state = LIGHTS_FOUND;
	return;
}