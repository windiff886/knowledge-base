#include"Armor.h"

using namespace cv;
using namespace std;

void eraseErrorRepeatArmor(vector<ArmorBox>& armors);

/**
* @brief:  将识别到的灯条拟合为装甲板
*/
void ArmorDetector::matchArmors() {
	for (int i = 0; i < lights.size() - 1; i++)
	{
		for (int j = i + 1; j < lights.size(); j++) //从左至右，每个灯条与其他灯条一次匹配判断
		{
			ArmorBox armor = ArmorBox(lights[i], lights[j]); //利用左右灯条构建装甲板
			if (armor.isSuitableArmor()) //如果是合适的装甲板，则设置其他装甲板信息
			{
				armor.l_index = i; // 左灯条的下标
				armor.r_index = j; // 右灯条的下标

				classifier.getArmorImg(armor);//装甲板的二值图
				classifier.setNumber(armor); //装甲板数字(数字模板)
				//classifier.setArmorNum(armor);// 装甲板数字(SVM)
				armors.emplace_back(armor); // 将匹配好的装甲板push入armors中
			}
		}

		eraseErrorRepeatArmor(armors);//删除游离灯条导致的错误装甲板
	}
	if (armors.empty()) {
		state = ARMOR_NOT_FOUND; //如果armors目前仍为空，则设置状态为ARMOR_NOT_FOUND
		return; 
	}
	else {
		state = ARMOR_FOUND; //如果非空（有装甲板）则设置状态ARMOR_FOUND
		return; 
	}
}

/**
 *@brief: 针对游离灯条导致的错误装甲板进行检测和删除
 */
void eraseErrorRepeatArmor(vector<ArmorBox>& armors)
{
	int length = armors.size();
	vector<ArmorBox>::iterator it = armors.begin();
	for (size_t i = 0; i < length; i++)
		for (size_t j = i + 1; j < length; j++)
		{
			if (armors[i].l_index == armors[j].l_index ||
				armors[i].l_index == armors[j].r_index ||
				armors[i].r_index == armors[j].l_index ||
				armors[i].r_index == armors[j].r_index)
			{
				armors[i].getDeviationAngle() > armors[j].getDeviationAngle() ? armors.erase(it + i) : armors.erase(it + j);
			}
		}
}