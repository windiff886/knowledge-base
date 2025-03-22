#ifndef ARMOR
#define ARMOR

#include<opencv.hpp>

enum ArmorType
{
	SMALL_ARMOR = 0,
	BIG_ARMOR = 1
};

enum DetectorState
{
	LIGHTS_NOT_FOUND = 0,
	LIGHTS_FOUND = 1,
	ARMOR_NOT_FOUND = 2,
	ARMOR_FOUND = 3
};

struct ArmorParam {

	int color_threshold;		//通道相减的colorImg使用的二值化阈值
	int bright_threshold;		//亮度图二值化阈值

	float min_area;				// 灯条允许的最小面积
	float max_angle;			// 灯条允许的最大偏角

	float max_angle_diff;		// 两个灯条之间允许的最大角度差
	float max_lengthDiff_ratio;	// 两个灯条之间允许的最大长度差比值
	float max_deviation_angle;	// 两灯条最大错位角

	float max_y_diff_ratio;		//max y 
	float max_x_diff_ratio;		//max x


	//default values  给各参数设定默认值
	ArmorParam() {
		color_threshold = 100 - 20;
		bright_threshold = 60;

		min_area = 50;
		max_angle = 45;

		max_angle_diff = 6;
		max_lengthDiff_ratio = 0.5;
		max_deviation_angle = 50;

		max_y_diff_ratio = 0.5;
		max_x_diff_ratio = 4.5;
	}
};

extern ArmorParam armorParam;

//装甲板两侧灯条的相关信息
class LightBar
{
public:

	LightBar();
	LightBar(const cv::RotatedRect& light);
	~LightBar();

public:
	cv::RotatedRect lightRect;	//灯条的旋转矩形（椭圆拟合获得）
	float length;				// 灯条长度
	cv::Point2f center;			// 灯条中心
	float angle;				// 灯条长度方向与竖直方向的夹角，左偏为0~90,右偏为0~-90
};

//装甲板相关数据信息
class ArmorBox
{
public:
	ArmorBox();
	ArmorBox(const LightBar& l_light, const LightBar& r_light);
	~ArmorBox();

	// 装甲板左右灯条角度差
	float getAngleDiff() const;

	// 灯条错位度角(两灯条中心连线与水平线夹角) 
	float getDeviationAngle() const;

	// 灯条位置差距 两灯条中心x方向差距比值
	float getDislocationX() const;

	//  灯条位置差距 两灯条中心Y方向差距比值
	float getDislocationY() const;

	// 左右灯条长度差比值
	float getLengthRation() const;

	// 判断是否适合
	bool isSuitableArmor() const;

public:
	LightBar l_light, r_light;		// 装甲板的左右灯条
	int l_index, r_index;			// 左右灯条的下标(默认为-1，仅作为ArmorDetector类成员时生效) 
	int armorNum;					//装甲板上的数字（用SVM识别得到）
	std::vector<cv::Point2f> armorVertices;  // 左下 左上 右上 右下
	ArmorType type;					//装甲板类型
	cv::Point2f center;				//  装甲板中心
	cv::Rect armorRect;				//装甲板的矩形获取roi用
	float armorAngle;				// 装甲板角度(灯条角度的平均值)
	cv::Mat armorImg;				//装甲板的图片（透射变换获得）
};

class ArmorNumClassifier
{
public:

	
	ArmorNumClassifier();
	~ArmorNumClassifier();
	/**
	 * @brief:  载入SVM模型（用于识别装甲板数字）
	 * @param: 待载入SVM模型的路径 模型的图片尺寸
	 */
	void loadSvmModel(const char* model_path, cv::Size armorImgSize = cv::Size(40, 40));


	//载入数字模板
	void loadTemplates(const std::string& filename);

	//数字模板实现装甲板数字识别
	void setNumber(ArmorBox& armor);

	/**
	 * @brief:  载入roiImage（剪切出装甲板）
	 * @param:   待载入SVM模型的路径
	 */
	void loadImg(cv::Mat& srcImg);

	/**
	 * @brief: use warpPerspective to get armorImg  利用透视变换获得装甲板图片
	 * @param: the path of xml_file  待载入SVM模型的路径
	 */
	void getArmorImg(ArmorBox& armor);

	/**
	 * @brief: 利用SVM实现装甲板数字识别
	 */
	void setArmorNum(ArmorBox& armor);
	

private:
	
	cv::Ptr<cv::ml::SVM>svm;		//模型
	cv::Mat p;						// 载入到SVM中识别的矩阵
	cv::Size armorImgSize;			//SVM模型的识别图片大小（训练集的图片大小）

	cv::Mat warpPerspective_src;	//  透射变换的原图
	cv::Mat warpPerspective_dst;	//   透射变换生成的目标图
	cv::Mat warpPerspective_mat;	// 透射变换的变换矩阵
	cv::Point2f srcPoints[4];		//	透射变换的原图上的目标点 tl->tr->br->bl  左上 右上 右下 左下
	cv::Point2f dstPoints[4];		//  透射变换的目标图中的点   tl->tr->br->bl  左上 右上 右下 左下
	
	std::vector<cv::Mat> templates;  //数字模板

};

enum Color
{
	BLUE = 0,
	GREEN = 1,
	RED = 2
};

/**
*@brief:装甲板识别类，实现装甲板两侧灯条的检测，
*		     装甲板的灯条匹配，装甲板的筛选，装甲板数字识别，选择目标等功能
*/
class ArmorDetector
{
public:
	ArmorDetector();
	~ArmorDetector();
	/**
	 * @brief: load svm model for client
	 * @param: the model file path of svm
	 */
	void loadSVM(const char* model_path, cv::Size armorImgSize = cv::Size(40, 40));

	/**
	 * @brief: 设置敌方颜色
	 */
	void setEnemyColor(Color enemyColor);

	/**
	 *@brief:重设检测器（删除原有的灯条和装甲板s）和装甲板状态，以便进行下一帧的检测
	 */
	void resetDetector();

	/**
	 * @brief: 载入源图像并进行图像预处理
	 * @param: 源图像的引用
	 */
	void setImg(cv::Mat& src);

	/**
	 * @brief:  检测所有可能的灯条
	 */
	void findLights();

	/**
	* @brief:  将识别到的灯条拟合为装甲板
	*/
	void matchArmors();

	/**
	 *@brief: 集成的装甲板检测识别函数
	 */
	cv::Mat run(cv::Mat& src);

	/**
	 *@brief:  识别程序是否识别到装甲版
	 *@return: FOUND(1) NOT_FOUND(0)
	 */
	bool isFoundArmor();

	void loadTemplate(const std::string& filename);

private:
	cv::Mat srcImg;					//当前的图像帧
	cv::Mat srcImg_binary;			//源图像的二值图
	Color enemyColor;				//敌方颜色
	std::vector<LightBar> lights;	// 找到的灯条
	std::vector<ArmorBox> armors;	// 识别到的所有装甲板
	ArmorNumClassifier classifier;	//获取装甲板图像及识别装甲板数字的类
	DetectorState state;			//装甲板检测器的状态，随着装甲板进程的执行而不断更新 
};

#endif