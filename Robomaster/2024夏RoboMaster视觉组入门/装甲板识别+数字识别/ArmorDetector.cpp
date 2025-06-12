#include "Armor.h"

using namespace cv;
using namespace std;

Mat showArmors(Mat& image, const vector<ArmorBox>& armors);

ArmorDetector::ArmorDetector()
{
	state = LIGHTS_NOT_FOUND;
}

ArmorDetector::~ArmorDetector() {}

void ArmorDetector::resetDetector()
{
	state = LIGHTS_NOT_FOUND;
	lights.clear();
	armors.clear();
}

/**
* @brief:  ���õз���ɫ
*/
void ArmorDetector::setEnemyColor(Color enemyColor) {
	this->enemyColor = enemyColor;
}

/**
* @brief: ����Դͼ������ROI���򣨵�ROIģʽ����������һ֡�ҵ�Ŀ��װ�װ�ʱ��
* @param: Դͼ�������
*/
void ArmorDetector::setImg(Mat& src) {
	src.copyTo(srcImg);  // �ֵ��������srcImg
	classifier.loadImg(srcImg);  //���뵱ǰ֡
	//classifier.loadImg(srcImg); //����classifier���Ա��srcImg������͸��任���г�װ�װ�ͼ
	srcImg_binary = Mat::zeros(srcImg.size(), CV_8UC1); //color feature image

	//ͨ����������Զ�����ʽ������ָ����ʣ���ȥ��split��substract��thresh������������1.7��
	//data of Mat  bgr bgr bgr bgr
	uchar* pdata = (uchar*)srcImg.data;
	uchar* qdata = (uchar*)srcImg_binary.data;
	int srcData = srcImg.rows * srcImg.cols;
	if (enemyColor == RED)
	{
		for (int i = 0; i < srcData; i++)
		{
			if (*(pdata + 2) - *pdata > armorParam.color_threshold)
				*qdata = 255;
			pdata += 3;
			qdata++;
		}
	}
	else if (enemyColor == BLUE)
	{
		for (int i = 0; i < srcData; i++)
		{
			if (*pdata - *(pdata + 2) > armorParam.color_threshold)
				*qdata = 255;
			pdata += 3;
			qdata++;
		}
	}

	Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(3, 3)); //���Ͳ���ʹ�õ���Ĥ
	dilate(srcImg_binary, srcImg_binary, kernel); //��roiIng_binary�������Ͳ������Եõ����������ƽ�����ν�
}

/**
 *@brief: ����svmģ��
 *@param: svmģ��·����ѵ������ͼ���ѧ
 */

void ArmorDetector::loadSVM(const char* model_path, Size armorImgSize)
{
	classifier.loadSvmModel(model_path, armorImgSize);
}


void ArmorDetector::loadTemplate(const string& filename) {
	classifier.loadTemplates(filename);
}

/**
 *@brief:  ������ArmorDetector
 */
Mat ArmorDetector::run(Mat& src) {
	//���ȣ����벢����ͼ��
	this->setImg(src); //����Detector��ȫ��Դͼ�� ����Դͼ��Ԥ����

	//�������detector�����ݣ��������һ֡���ҵ��ĵ�����װ�װ壬ͬʱ�����״̬����ΪLIGHTS_NOT_FOUND�����״̬��
	resetDetector();

	//���������ڵ�ǰͼ�����ҳ����еĵ���
	findLights();

	//���Ĳ������״̬ΪLIGHTS_FOUND���ҵ�������������������
	if (state == LIGHTS_FOUND)
	{
		//��ÿ��������ƥ��Ϊһ��װ�װ壬���ƥ�������װ�װ��Ǻ��ʵģ���ѹ��armors��
		matchArmors();
	}

	//���岽,���ߺ�����
	Mat final = showArmors(srcImg, armors);
	return final;
	
}

/**
 *@brief: ʶ������Ƿ�ʶ��װ�װ�
 *@return: FOUND(1) NOT_FOUND(0)
 */
bool ArmorDetector::isFoundArmor()
{
	if (state == ARMOR_FOUND)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


///////////////////////////////////////////////////////////  functions  for   debugging      //////////////////////////////////////////////////////////////////

/**
 *@brief:  ��ͼ������ʾ�ҵ������е���
 */
void showLights(Mat& image, const vector<LightBar>& lights)
{
	Mat lightDisplay;//��ʾ�����õ�ͼ��
	image.copyTo(lightDisplay);// ��ȡԴͼ��Ŀ���
	// ����ҵ��˵���
	if (!lights.empty())
	{
		putText(lightDisplay, "LIGHTS FOUND!", Point(100, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 255), 1, 8, false); //����� ���ҵ��˵�����
		for (auto light : lights)
		{
			Point2f lightVertices[4];
			light.lightRect.points(lightVertices);
			//�������е���������
			for (size_t i = 0; i < 4; i++)
			{
				line(lightDisplay, lightVertices[i], lightVertices[(i + 1) % 4], Scalar(255, 0, 255), 1, 8, 0);
			}

			// ������������
			circle(lightDisplay, light.center, 2, Scalar(0, 255, 0), 2, 8, 0);

			// ��ʾ���������������
			putText(lightDisplay, to_string(int(light.center.x)), light.center, FONT_HERSHEY_PLAIN, 1, Scalar(0, 255, 0), 1, 8, false);
			putText(lightDisplay, to_string(int(light.center.y)), light.center + Point2f(0, 15), FONT_HERSHEY_PLAIN, 1, Scalar(0, 255, 0), 1, 8, false);
		}
	}
	//���û�ҵ�����
	else
	{
		putText(lightDisplay, "LIGHTS NOT FOUND!", Point(100, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 1, 8, false);//title LIGHT_NOT_FOUND ����� ��û�ҵ�������
	}
	// ��ʾ���ͼ
	imshow("Lights Monitor", lightDisplay);
}

/**
 *@brief: ��ͼ������ʾ�ҵ�������װ�װ�
 */
Mat showArmors(Mat& image, const vector<ArmorBox>& armors)
{
	Mat armorDisplay; //չʾװ�װ��ͼ��
	image.copyTo(armorDisplay); //Դͼ��Ŀ��� 
	// ����ҵ���װ�װ�
	if (!armors.empty())
	{
		putText(armorDisplay, "ARMOR FOUND!", Point(100, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 0), 1, 8, false); //title FOUND ����� ���ҵ���װ�װ塱
		//��������װ�װ�Ķ���ߺ�����
		for (auto armor : armors)
		{
			if (armor.armorNum == -1) {
				continue;
			}
			//������
			circle(armorDisplay, armor.center, 2, Scalar(0, 255, 0), 2);
			for (size_t i = 0; i < 4; i++)
			{
				line(armorDisplay, armor.armorVertices[i], armor.armorVertices[(i + 1) % 4], Scalar(255, 255, 0), 2, 8, 0);
			}
			// ��ʾ����
			//putText(armorDisplay, to_string(int(armor.center.x)), armor.center, FONT_HERSHEY_PLAIN, 1, Scalar(255, 0, 255), 1, 8, false);
			//putText(armorDisplay, to_string(int(armor.center.y)), armor.center + Point2f(0, 15), FONT_HERSHEY_PLAIN, 1, Scalar(255, 0, 255), 1, 8, false);
			putText(armorDisplay, to_string(int(armor.armorNum)), armor.center /* + Point2f(15, 30)*/, FONT_HERSHEY_PLAIN, 4, Scalar(255, 255, 255), 4, 8, false);
		}
		//����װ�װ�����
		for (int i = 0; i < armors.size(); i++) {
			for (size_t j = 0; j < 4; j++) {
				line(armorDisplay, armors[i].armorVertices[j], armors[i].armorVertices[(j + 1) % 4], Scalar(255, 255, 255), 2, 8, 0);
			}
		}
	}
	// ���û�ҵ�װ�װ�
	else
	{
		putText(armorDisplay, "ARMOR NOT FOUND!", Point(100, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 0, 255), 1, 8, false);//title NOT FOUND ����� ��û�ҵ�װ�װ塱
	}

	//��ʾ���ͼ
	Size originalSize = armorDisplay.size();
	Size newSize(originalSize.width / 2, originalSize.height / 2);
	Mat resizedFrame;
	resize(armorDisplay, resizedFrame, newSize, 0, 0, cv::INTER_AREA);
	imshow("Armor Monitor", resizedFrame);
	return armorDisplay;
}

/**
 *@brief: �ڿ���̨����ҵ����������ĺͽǶ�
 */
void textLights(vector<LightBar>& lights)
{
	cout << "\n################## L I G H T S ##################" << endl;
	if (lights.empty()) {
		cout << "LIGHTS NOT FOUND!" << endl;
	}
	else
	{
		cout << "LIGHTS FOUND!" << endl;
		for (size_t i = 0; i < lights.size(); i++)
		{
			cout << "#############################" << endl;
			cout << "Light Center:" << lights[i].center << endl;
			cout << "Light Angle:" << lights[i].angle << endl;
		}
		cout << "#################################################" << endl;
	}
}

/**
 *@brief: �ڿ���̨����ҵ�װ�װ�����ġ����֡�ƥ����Ϣ
 */
void textArmors(vector<ArmorBox>& armors)
{
	cout << "\n$$$$$$$$$$$$$$$$$$ A R M O R S $$$$$$$$$$$$$$$$$$" << endl;
	if (armors.empty()) {
		cout << "ARMORS NOT FOUND!" << endl;
	}
	else
	{
		cout << "ARMOR FOUND!" << endl;
		for (size_t i = 0; i < armors.size(); i++)
		{
			cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << endl;
			cout << "Armor Center: " << armors[i].center << endl;
			cout << "Armor Number: " << armors[i].armorNum << endl;
			if (armors[i].type == SMALL_ARMOR) cout << "Armor Type: SMALL ARMOR" << endl;
			else if (armors[i].type == BIG_ARMOR) cout << "Armor Type: BIG ARMOR" << endl;
			cout << "\n###### matching information ######" << endl;
			cout << "Angle difference: " << armors[i].getAngleDiff() << endl;
			cout << "Deviation Angle: " << armors[i].getDeviationAngle() << endl;
			cout << "X Dislocation Ration: " << armors[i].getDislocationX() << endl;
			cout << "Y Dislocation Ration: " << armors[i].getDislocationY() << endl;
			cout << "Length Ration: " << armors[i].getLengthRation() << endl;
		}
		cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << endl;
	}

}