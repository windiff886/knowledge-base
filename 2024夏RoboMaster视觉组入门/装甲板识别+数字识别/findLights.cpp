#include"Armor.h"
using namespace cv;
using namespace std;

/**
* @brief:  ������п��ܵĵ���
*/
void ArmorDetector::findLights() {
	vector<vector<Point>> lightContours;  //��ѡ��������
	Mat contourImg; //��findContours�õ�ͼ�񣬷�ֹfindContours�ı�roiImg
	srcImg_binary.copyTo(contourImg); //a copy of roiImg, contourImg
	findContours(contourImg, lightContours, 0, 2); //CV_RETR_EXTERNAL = 0, CV_CHAIN_APPROX_SIMPLE = 2       ���ʱ�Ĳ������Ż�����
	RotatedRect lightRect;  // �����Բ���ĵ�����ת����
	LightBar light;  //��ʱ����
	for (const auto& lightContour : lightContours) {
		if (lightContour.size() < 6) continue; // ��������С��6�����������Բ
		if (contourArea(lightContour) < armorParam.min_area) continue; //���ɸѡ��ȥС�����

		lightRect = fitEllipse(lightContour); //�����Բ
		light = LightBar(lightRect);//����Ϊ����

		if (abs(light.angle) > armorParam.max_angle) continue; //�Ƕ�ɸѡ����ȥһЩ��ֱƫ��ƫ���

		lights.emplace_back(light);
	}
	if (lights.size() < 2) {
		state = LIGHTS_NOT_FOUND; //������������������״̬Ϊû�ҵ�����
		return; 
	}

	// ����������������
	sort(lights.begin(), lights.end(),
		[](LightBar& a1, LightBar& a2) {
			return a1.center.x < a2.center.x; });
	state = LIGHTS_FOUND;
	return;
}