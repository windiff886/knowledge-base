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
* @brief:  设置敌方颜色
*/
void ArmorDetector::setEnemyColor(Color enemyColor) {
	this->enemyColor = enemyColor;
}

/**
* @brief: 载入源图像并设置ROI区域（当ROI模式开启，且上一帧找到目标装甲板时）
* @param: 源图像的引用
*/
void ArmorDetector::setImg(Mat& src) {
	src.copyTo(srcImg);  // 深（值）拷贝给srcImg
	classifier.loadImg(srcImg);  //载入当前帧
	//classifier.loadImg(srcImg); //载入classifier类成员的srcImg，用于透射变换剪切出装甲板图
	srcImg_binary = Mat::zeros(srcImg.size(), CV_8UC1); //color feature image

	//通道相减法的自定义形式，利用指针访问，免去了split、substract和thresh操作，加速了1.7倍
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

	Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(3, 3)); //膨胀操作使用的掩膜
	dilate(srcImg_binary, srcImg_binary, kernel); //对roiIng_binary进行膨胀操作，试得灯条区域更加平滑有衔接
}

/**
 *@brief: 载入svm模型
 *@param: svm模型路径及训练集的图像大学
 */

void ArmorDetector::loadSVM(const char* model_path, Size armorImgSize)
{
	classifier.loadSvmModel(model_path, armorImgSize);
}


void ArmorDetector::loadTemplate(const string& filename) {
	classifier.loadTemplates(filename);
}

/**
 *@brief:  集成跑ArmorDetector
 */
Mat ArmorDetector::run(Mat& src) {
	//首先，载入并处理图像
	this->setImg(src); //载入Detector的全局源图像 并对源图像预处理

	//随后，重设detector的内容，清空在上一帧中找到的灯条和装甲板，同时检测器状态重置为LIGHTS_NOT_FOUND（最低状态）
	resetDetector();

	//第三步，在当前图像中找出所有的灯条
	findLights();

	//第四步，如果状态为LIGHTS_FOUND（找到多于两个灯条），则
	if (state == LIGHTS_FOUND)
	{
		//将每两个灯条匹配为一个装甲板，如果匹配出来的装甲板是合适的，则压入armors中
		matchArmors();
	}

	//第五步,画线和数字
	Mat final = showArmors(srcImg, armors);
	return final;
	
}

/**
 *@brief: 识别程序是否识别到装甲版
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
 *@brief:  在图像中显示找到的所有灯条
 */
void showLights(Mat& image, const vector<LightBar>& lights)
{
	Mat lightDisplay;//显示灯条用的图像
	image.copyTo(lightDisplay);// 获取源图像的拷贝
	// 如果找到了灯条
	if (!lights.empty())
	{
		putText(lightDisplay, "LIGHTS FOUND!", Point(100, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 255), 1, 8, false); //大标题 “找到了灯条”
		for (auto light : lights)
		{
			Point2f lightVertices[4];
			light.lightRect.points(lightVertices);
			//画出所有灯条的轮廓
			for (size_t i = 0; i < 4; i++)
			{
				line(lightDisplay, lightVertices[i], lightVertices[(i + 1) % 4], Scalar(255, 0, 255), 1, 8, 0);
			}

			// 画出灯条中心
			circle(lightDisplay, light.center, 2, Scalar(0, 255, 0), 2, 8, 0);

			// 显示灯条的中心坐标点
			putText(lightDisplay, to_string(int(light.center.x)), light.center, FONT_HERSHEY_PLAIN, 1, Scalar(0, 255, 0), 1, 8, false);
			putText(lightDisplay, to_string(int(light.center.y)), light.center + Point2f(0, 15), FONT_HERSHEY_PLAIN, 1, Scalar(0, 255, 0), 1, 8, false);
		}
	}
	//如果没找到灯条
	else
	{
		putText(lightDisplay, "LIGHTS NOT FOUND!", Point(100, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 1, 8, false);//title LIGHT_NOT_FOUND 大标题 “没找到灯条”
	}
	// 显示结果图
	imshow("Lights Monitor", lightDisplay);
}

/**
 *@brief: 在图像中显示找到的所有装甲板
 */
Mat showArmors(Mat& image, const vector<ArmorBox>& armors)
{
	Mat armorDisplay; //展示装甲板的图像
	image.copyTo(armorDisplay); //源图像的拷贝 
	// 如果找到了装甲板
	if (!armors.empty())
	{
		putText(armorDisplay, "ARMOR FOUND!", Point(100, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 0), 1, 8, false); //title FOUND 大标题 “找到了装甲板”
		//画出所有装甲板的顶点边和中心
		for (auto armor : armors)
		{
			if (armor.armorNum == -1) {
				continue;
			}
			//画中心
			circle(armorDisplay, armor.center, 2, Scalar(0, 255, 0), 2);
			for (size_t i = 0; i < 4; i++)
			{
				line(armorDisplay, armor.armorVertices[i], armor.armorVertices[(i + 1) % 4], Scalar(255, 255, 0), 2, 8, 0);
			}
			// 显示数字
			//putText(armorDisplay, to_string(int(armor.center.x)), armor.center, FONT_HERSHEY_PLAIN, 1, Scalar(255, 0, 255), 1, 8, false);
			//putText(armorDisplay, to_string(int(armor.center.y)), armor.center + Point2f(0, 15), FONT_HERSHEY_PLAIN, 1, Scalar(255, 0, 255), 1, 8, false);
			putText(armorDisplay, to_string(int(armor.armorNum)), armor.center /* + Point2f(15, 30)*/, FONT_HERSHEY_PLAIN, 4, Scalar(255, 255, 255), 4, 8, false);
		}
		//画出装甲板轮廓
		for (int i = 0; i < armors.size(); i++) {
			for (size_t j = 0; j < 4; j++) {
				line(armorDisplay, armors[i].armorVertices[j], armors[i].armorVertices[(j + 1) % 4], Scalar(255, 255, 255), 2, 8, 0);
			}
		}
	}
	// 如果没找到装甲板
	else
	{
		putText(armorDisplay, "ARMOR NOT FOUND!", Point(100, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 0, 255), 1, 8, false);//title NOT FOUND 大标题 “没找到装甲板”
	}

	//显示结果图
	Size originalSize = armorDisplay.size();
	Size newSize(originalSize.width / 2, originalSize.height / 2);
	Mat resizedFrame;
	resize(armorDisplay, resizedFrame, newSize, 0, 0, cv::INTER_AREA);
	imshow("Armor Monitor", resizedFrame);
	return armorDisplay;
}

/**
 *@brief: 在控制台输出找到灯条的中心和角度
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
 *@brief: 在控制台输出找到装甲板的中心、数字、匹配信息
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