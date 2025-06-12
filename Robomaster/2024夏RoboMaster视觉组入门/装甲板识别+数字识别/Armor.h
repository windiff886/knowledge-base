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

	int color_threshold;		//ͨ�������colorImgʹ�õĶ�ֵ����ֵ
	int bright_threshold;		//����ͼ��ֵ����ֵ

	float min_area;				// �����������С���
	float max_angle;			// ������������ƫ��

	float max_angle_diff;		// ��������֮����������ǶȲ�
	float max_lengthDiff_ratio;	// ��������֮���������󳤶Ȳ��ֵ
	float max_deviation_angle;	// ����������λ��

	float max_y_diff_ratio;		//max y 
	float max_x_diff_ratio;		//max x


	//default values  ���������趨Ĭ��ֵ
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

//װ�װ���������������Ϣ
class LightBar
{
public:

	LightBar();
	LightBar(const cv::RotatedRect& light);
	~LightBar();

public:
	cv::RotatedRect lightRect;	//��������ת���Σ���Բ��ϻ�ã�
	float length;				// ��������
	cv::Point2f center;			// ��������
	float angle;				// �������ȷ�������ֱ����ļнǣ���ƫΪ0~90,��ƫΪ0~-90
};

//װ�װ����������Ϣ
class ArmorBox
{
public:
	ArmorBox();
	ArmorBox(const LightBar& l_light, const LightBar& r_light);
	~ArmorBox();

	// װ�װ����ҵ����ǶȲ�
	float getAngleDiff() const;

	// ������λ�Ƚ�(����������������ˮƽ�߼н�) 
	float getDeviationAngle() const;

	// ����λ�ò�� ����������x�������ֵ
	float getDislocationX() const;

	//  ����λ�ò�� ����������Y�������ֵ
	float getDislocationY() const;

	// ���ҵ������Ȳ��ֵ
	float getLengthRation() const;

	// �ж��Ƿ��ʺ�
	bool isSuitableArmor() const;

public:
	LightBar l_light, r_light;		// װ�װ�����ҵ���
	int l_index, r_index;			// ���ҵ������±�(Ĭ��Ϊ-1������ΪArmorDetector���Աʱ��Ч) 
	int armorNum;					//װ�װ��ϵ����֣���SVMʶ��õ���
	std::vector<cv::Point2f> armorVertices;  // ���� ���� ���� ����
	ArmorType type;					//װ�װ�����
	cv::Point2f center;				//  װ�װ�����
	cv::Rect armorRect;				//װ�װ�ľ��λ�ȡroi��
	float armorAngle;				// װ�װ�Ƕ�(�����Ƕȵ�ƽ��ֵ)
	cv::Mat armorImg;				//װ�װ��ͼƬ��͸��任��ã�
};

class ArmorNumClassifier
{
public:

	
	ArmorNumClassifier();
	~ArmorNumClassifier();
	/**
	 * @brief:  ����SVMģ�ͣ�����ʶ��װ�װ����֣�
	 * @param: ������SVMģ�͵�·�� ģ�͵�ͼƬ�ߴ�
	 */
	void loadSvmModel(const char* model_path, cv::Size armorImgSize = cv::Size(40, 40));


	//��������ģ��
	void loadTemplates(const std::string& filename);

	//����ģ��ʵ��װ�װ�����ʶ��
	void setNumber(ArmorBox& armor);

	/**
	 * @brief:  ����roiImage�����г�װ�װ壩
	 * @param:   ������SVMģ�͵�·��
	 */
	void loadImg(cv::Mat& srcImg);

	/**
	 * @brief: use warpPerspective to get armorImg  ����͸�ӱ任���װ�װ�ͼƬ
	 * @param: the path of xml_file  ������SVMģ�͵�·��
	 */
	void getArmorImg(ArmorBox& armor);

	/**
	 * @brief: ����SVMʵ��װ�װ�����ʶ��
	 */
	void setArmorNum(ArmorBox& armor);
	

private:
	
	cv::Ptr<cv::ml::SVM>svm;		//ģ��
	cv::Mat p;						// ���뵽SVM��ʶ��ľ���
	cv::Size armorImgSize;			//SVMģ�͵�ʶ��ͼƬ��С��ѵ������ͼƬ��С��

	cv::Mat warpPerspective_src;	//  ͸��任��ԭͼ
	cv::Mat warpPerspective_dst;	//   ͸��任���ɵ�Ŀ��ͼ
	cv::Mat warpPerspective_mat;	// ͸��任�ı任����
	cv::Point2f srcPoints[4];		//	͸��任��ԭͼ�ϵ�Ŀ��� tl->tr->br->bl  ���� ���� ���� ����
	cv::Point2f dstPoints[4];		//  ͸��任��Ŀ��ͼ�еĵ�   tl->tr->br->bl  ���� ���� ���� ����
	
	std::vector<cv::Mat> templates;  //����ģ��

};

enum Color
{
	BLUE = 0,
	GREEN = 1,
	RED = 2
};

/**
*@brief:װ�װ�ʶ���࣬ʵ��װ�װ���������ļ�⣬
*		     װ�װ�ĵ���ƥ�䣬װ�װ��ɸѡ��װ�װ�����ʶ��ѡ��Ŀ��ȹ���
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
	 * @brief: ���õз���ɫ
	 */
	void setEnemyColor(Color enemyColor);

	/**
	 *@brief:����������ɾ��ԭ�еĵ�����װ�װ�s����װ�װ�״̬���Ա������һ֡�ļ��
	 */
	void resetDetector();

	/**
	 * @brief: ����Դͼ�񲢽���ͼ��Ԥ����
	 * @param: Դͼ�������
	 */
	void setImg(cv::Mat& src);

	/**
	 * @brief:  ������п��ܵĵ���
	 */
	void findLights();

	/**
	* @brief:  ��ʶ�𵽵ĵ������Ϊװ�װ�
	*/
	void matchArmors();

	/**
	 *@brief: ���ɵ�װ�װ���ʶ����
	 */
	cv::Mat run(cv::Mat& src);

	/**
	 *@brief:  ʶ������Ƿ�ʶ��װ�װ�
	 *@return: FOUND(1) NOT_FOUND(0)
	 */
	bool isFoundArmor();

	void loadTemplate(const std::string& filename);

private:
	cv::Mat srcImg;					//��ǰ��ͼ��֡
	cv::Mat srcImg_binary;			//Դͼ��Ķ�ֵͼ
	Color enemyColor;				//�з���ɫ
	std::vector<LightBar> lights;	// �ҵ��ĵ���
	std::vector<ArmorBox> armors;	// ʶ�𵽵�����װ�װ�
	ArmorNumClassifier classifier;	//��ȡװ�װ�ͼ��ʶ��װ�װ����ֵ���
	DetectorState state;			//װ�װ�������״̬������װ�װ���̵�ִ�ж����ϸ��� 
};

#endif