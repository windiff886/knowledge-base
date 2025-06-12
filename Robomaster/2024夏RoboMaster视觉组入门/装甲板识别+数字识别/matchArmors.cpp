#include"Armor.h"

using namespace cv;
using namespace std;

void eraseErrorRepeatArmor(vector<ArmorBox>& armors);

/**
* @brief:  ��ʶ�𵽵ĵ������Ϊװ�װ�
*/
void ArmorDetector::matchArmors() {
	for (int i = 0; i < lights.size() - 1; i++)
	{
		for (int j = i + 1; j < lights.size(); j++) //�������ң�ÿ����������������һ��ƥ���ж�
		{
			ArmorBox armor = ArmorBox(lights[i], lights[j]); //�������ҵ�������װ�װ�
			if (armor.isSuitableArmor()) //����Ǻ��ʵ�װ�װ壬����������װ�װ���Ϣ
			{
				armor.l_index = i; // ��������±�
				armor.r_index = j; // �ҵ������±�

				classifier.getArmorImg(armor);//װ�װ�Ķ�ֵͼ
				classifier.setNumber(armor); //װ�װ�����(����ģ��)
				//classifier.setArmorNum(armor);// װ�װ�����(SVM)
				armors.emplace_back(armor); // ��ƥ��õ�װ�װ�push��armors��
			}
		}

		eraseErrorRepeatArmor(armors);//ɾ������������µĴ���װ�װ�
	}
	if (armors.empty()) {
		state = ARMOR_NOT_FOUND; //���armorsĿǰ��Ϊ�գ�������״̬ΪARMOR_NOT_FOUND
		return; 
	}
	else {
		state = ARMOR_FOUND; //����ǿգ���װ�װ壩������״̬ARMOR_FOUND
		return; 
	}
}

/**
 *@brief: �������������µĴ���װ�װ���м���ɾ��
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