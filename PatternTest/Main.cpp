#include <iostream>
#include <iterator>
#include <time.h>
#include "ReadInTrajs.h"
#include "Map.h"
#include "TimeSlice.h"
#include "NewTimeSlice.h"
#include "Parameters.h"
#include "Evaluation.h"
using namespace std;

Map routeNetwork;
string filePath = "D:\\MapMatchingProject\\Data\\�¼�������\\";
vector<NewTimeSlice*> timeSlices;
list<list<EdgeCluster*>> resultsList;//���

//�Ա�ʵ��׼����������ȡ�켣�ļ�����������������
vector<TimeSlice*> clusterDemo(){
	vector<TimeSlice*> timeSlices = vector<TimeSlice*>(1440);//��ʼ��ʱ��Ƭ����
	int timeStamp = 0;
	for (int timeStamp = 0; timeStamp < 1440; timeStamp++){
		timeSlices.at(timeStamp) = new TimeSlice(timeStamp);
	}
	scanTrajFolder(filePath, timeSlices);//����켣
	cout << "�������й켣" << endl;
	int outIndexCount = 0;
	for each (TimeSlice* timeSlice in timeSlices)//�Թ켣�����㽨������
	{
		for each (GeoPoint* point in (*timeSlice).points)
		{
			if (!routeNetwork.insertPoint(point)){
				outIndexCount++;
			}
		}
	}
	cout << "�Թ켣����������ϣ�" << endl;
	cout << "����" << outIndexCount << "����������������Χ��" << endl;
	for each (TimeSlice* timeSlice in timeSlices)
	{
		timeSlice->clustering(routeNetwork);
	}
	//���ĳ��ʱ��Ƭ�ľ�����
	timeSlices[10 * 60]->clustering(routeNetwork);
	outputDBSCANResult(timeSlices[10 * 60]->clusters);
	return timeSlices;
}

//���TimeSlice
void outputTimeSlices(vector<TimeSlice*> &timeSlices){
	ofstream fout("DBScanResult_day5.txt");
	for (auto timeSlice : timeSlices){
		fout << timeSlice->time << ":" << timeSlice->clusters.size() << endl;
		for (auto cluster : timeSlice->clusters){
			for (auto object : cluster->objectIds){
				fout << object << " ";
			}
			fout << endl;
		}
	}
	fout.close();
}

//ʵ��׼����������ȡ��ͼƥ��������ɸ�ʱ��Ƭ��·�ξ���
void edgeCluster(){
	timeSlices = vector<NewTimeSlice*>(1440);//��ʼ��ʱ��Ƭ����
	int timeStamp = 0;
	for (int timeStamp = 0; timeStamp < 1440; timeStamp++){
		timeSlices.at(timeStamp) = new NewTimeSlice(timeStamp);
	}
	scanMapMatchingResultFolder(filePath, timeSlices, routeNetwork);//�����ͼƥ�����ļ������ʱ��Ƭ��·�ξ���
	cout << "�������е�ͼƥ����" << endl;
	for (auto timeSlice : timeSlices)
	{
		for (auto edgeCluster : timeSlice->clusters){
			edgeCluster.second->ascertainPriorCanadidates();
		}
	}
}

//naive�����ĸ����������жϸ���������·�ξ����Ƿ�������չ���������㷵��true�����򷵻�false
bool couldExtendOrNot(set<int> &set1, set<int> &set2, int &intersectionCount){
	set<int> unionResult = set<int>();
	set_union(set1.begin(), set1.end(), set2.begin(), set2.end(), inserter(unionResult, unionResult.begin()));
	set<int> intersectionResult = set<int>();
	set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(), inserter(intersectionResult, intersectionResult.begin()));
	intersectionCount = intersectionResult.size();
	double similarity = (intersectionResult.size() + 0.0) / unionResult.size();
	return similarity >= DE_MINSIMILARITY;
}

//naive�����ĸ�������������һʱ��Ƭ����չ������·�ξ���
list<EdgeCluster*> extendDensityEdges(EdgeCluster* edgeCluster){
	list<EdgeCluster*> result = list<EdgeCluster*>();
	if (edgeCluster->time < timeSlices.size() - 1){
		for (auto tmpCluster : timeSlices.at(edgeCluster->time + 1)->clusters)
		{
			//������СԪ�ظ��������Ϳ���չ������
			int useless = 0;
			if (tmpCluster.second->clusterObjects.size() >= DE_MINOBJECTS&&couldExtendOrNot(edgeCluster->clusterObjects, tmpCluster.second->clusterObjects, useless)){
				tmpCluster.second->assigned = true;
				result.push_back(tmpCluster.second);
			}
		}
	}
	return result;
}

//naive����
list<list<EdgeCluster*>> naiveMethod(){
	resultsList = list<list<EdgeCluster*>>();
	list<list<EdgeCluster*>> canadidates = list<list<EdgeCluster*>>();//��ѡ���м���
	for (auto timeSlice : timeSlices){
		list<list<EdgeCluster*>> newCanadidates = list<list<EdgeCluster*>>();//�µĺ�ѡ���м���
		for (auto canadidate : canadidates){
			EdgeCluster* lastSnapshotCluster = canadidate.back();
			list<EdgeCluster*> assignedEdgeClusters = extendDensityEdges(lastSnapshotCluster);
			if (assignedEdgeClusters.size() == 0){
				if (canadidate.size() >= DE_DURATIVE){//�������������
					resultsList.push_back(canadidate);
				}
			}
			else{
				for (auto assignedEdgeCluster : assignedEdgeClusters){
					newCanadidates.push_back(canadidate);
					newCanadidates.back().push_back(assignedEdgeCluster);
				}
			}
		}
		for (auto edgeCluster : timeSlice->clusters){
			//��ǰʱ��Ƭ�е�·�ξ���δ������չ��������СԪ�ظ�������������Ϊ�µ�һ����ѡ����
			if ((!edgeCluster.second->assigned) && edgeCluster.second->clusterObjects.size() >= DE_MINOBJECTS){
				edgeCluster.second->assigned = true;
				list<EdgeCluster*> densityEdges = list<EdgeCluster*>();
				densityEdges.push_back(edgeCluster.second);
				newCanadidates.push_back(densityEdges);
			}
		}
		canadidates = newCanadidates;//���º�ѡ���м���
	}
	return resultsList;
}

//��kֵ��֦�ķ����ĸ�������������һʱ��Ƭ����չ������·�ξ���
//k��֦��(1-DE_SIMILARITY)*|cluster|>=k
list<EdgeCluster*> extendDensityEdgesWithKPruning(EdgeCluster* edgeCluster){
	list<EdgeCluster*> result = list<EdgeCluster*>();
	if (edgeCluster->time < timeSlices.size() - 1){
		for (auto tmpCluster : timeSlices.at(edgeCluster->time + 1)->clusters)
		{
			//���ڴ���չʱ��Ƭ�е�·�ξ��ֻ࣬�в�����k��֦�����ſ��ܳ�Ϊ���е���һ��Ԫ��
			if (tmpCluster.second->k <= tmpCluster.second->clusterObjects.size()*(1 - DE_MINSIMILARITY)){
				int intersectionCount = 0;
				//������СԪ�ظ��������Ϳ���չ������
				if (tmpCluster.second->clusterObjects.size() >= DE_MINOBJECTS&&couldExtendOrNot(edgeCluster->clusterObjects, tmpCluster.second->clusterObjects, intersectionCount)){
					tmpCluster.second->assigned = true;
					tmpCluster.second->k += intersectionCount;
					edgeCluster->k += intersectionCount;
					result.push_back(tmpCluster.second);
				}
				if (edgeCluster->k > edgeCluster->clusterObjects.size()*(1 - DE_MINSIMILARITY)){//���ڵ�ǰ���е����һ�����࣬һ������k��֦����ֹͣ��չ
					break;
				}
			}
			else{
				continue;
			}
		}
	}
	return result;
}

//��kֵ��֦�ķ���
list<list<EdgeCluster*>> methodWithKPruning(){
	resultsList = list<list<EdgeCluster*>>();
	list<list<EdgeCluster*>> canadidates = list<list<EdgeCluster*>>();//��ѡ���м���
	for (auto timeSlice : timeSlices){
		list<list<EdgeCluster*>> newCanadidates = list<list<EdgeCluster*>>();//�µĺ�ѡ���м���
		for (auto canadidate : canadidates){
			EdgeCluster* lastSnapshotCluster = canadidate.back();
			lastSnapshotCluster->k = 0;
			list<EdgeCluster*> assignedEdgeClusters = extendDensityEdgesWithKPruning(lastSnapshotCluster);
			if (assignedEdgeClusters.size() == 0){
				if (canadidate.size() >= DE_DURATIVE){//�������������
					resultsList.push_back(canadidate);
				}
			}
			else{
				for (auto assignedEdgeCluster : assignedEdgeClusters){
					newCanadidates.push_back(canadidate);
					newCanadidates.back().push_back(assignedEdgeCluster);
				}
			}
		}
		for (auto edgeCluster : timeSlice->clusters){
			//��ǰʱ��Ƭ�е�·�ξ���δ������չ��������СԪ�ظ�������������Ϊ�µ�һ����ѡ����
			if ((!edgeCluster.second->assigned) && edgeCluster.second->clusterObjects.size() >= DE_MINOBJECTS){
				edgeCluster.second->assigned = true;
				list<EdgeCluster*> densityEdges = list<EdgeCluster*>();
				densityEdges.push_back(edgeCluster.second);
				newCanadidates.push_back(densityEdges);
			}
		}
		canadidates = newCanadidates;//���º�ѡ���м���
	}
	return resultsList;
}

//���ú���ƥ��·����Ϣ�ķ����ĸ�������������һʱ��Ƭ����չ������·�ξ���
//���ú���ƥ��·����Ϣ����֮ǰ����ö����һʱ��Ƭ�е�����·�ξ��಻ͬ��ֻö�ٵ�ǰ·�ξ����������켣�ĺ���ƥ��·�����ڵ�·�ξ���
list<EdgeCluster*> extendDensityEdgesWithKPruningAndMoreInfo(EdgeCluster* edgeCluster){
	list<EdgeCluster*> result = list<EdgeCluster*>();
	if (edgeCluster->time < timeSlices.size() - 1){
		for (auto edge : edgeCluster->priorCanadidates){//�ʹ�kֵ��֦�ķ�����ȣ�������仰��һ������仰�Ժ���һ����
			EdgeCluster* tmpCluster = timeSlices.at(edgeCluster->time + 1)->clusters.at(edge);
			//���ڴ���չʱ��Ƭ�е�·�ξ��ֻ࣬�в�����k��֦�����ſ��ܳ�Ϊ���е���һ��Ԫ��
			if (tmpCluster->k <= tmpCluster->clusterObjects.size()*(1 - DE_MINSIMILARITY)){
				int intersectionCount = 0;
				//������СԪ�ظ��������Ϳ���չ������
				if (tmpCluster->clusterObjects.size() >= DE_MINOBJECTS&&couldExtendOrNot(edgeCluster->clusterObjects, tmpCluster->clusterObjects, intersectionCount)){
					tmpCluster->assigned = true;
					tmpCluster->k += intersectionCount;
					edgeCluster->k += intersectionCount;
					result.push_back(tmpCluster);
				}
				if (edgeCluster->k > edgeCluster->clusterObjects.size()*(1 - DE_MINSIMILARITY)){//���ڵ�ǰ���е����һ�����࣬һ������k��֦����ֹͣ��չ
					break;
				}
			}
			else{
				continue;
			}
		}
	}
	return result;
}

//���ú���ƥ��·����Ϣ�ķ���
list<list<EdgeCluster*>> methodWithKPruningAndMoreInfo(){
	resultsList = list<list<EdgeCluster*>>();
	list<list<EdgeCluster*>> canadidates = list<list<EdgeCluster*>>();//��ѡ���м���
	for (auto timeSlice : timeSlices){
		list<list<EdgeCluster*>> newCanadidates = list<list<EdgeCluster*>>();//�µĺ�ѡ���м���
		for (auto canadidate : canadidates){
			EdgeCluster* lastSnapshotCluster = canadidate.back();
			lastSnapshotCluster->k = 0;
			list<EdgeCluster*> assignedEdgeClusters = extendDensityEdgesWithKPruningAndMoreInfo(lastSnapshotCluster);
			if (assignedEdgeClusters.size() == 0){
				if (canadidate.size() >= DE_DURATIVE){//�������������
					resultsList.push_back(canadidate);
				}
			}
			else{
				for (auto assignedEdgeCluster : assignedEdgeClusters){
					newCanadidates.push_back(canadidate);
					newCanadidates.back().push_back(assignedEdgeCluster);
				}
			}
		}
		for (auto edgeCluster : timeSlice->clusters){
			//��ǰʱ��Ƭ�е�·�ξ���δ������չ��������СԪ�ظ�������������Ϊ�µ�һ����ѡ����
			if ((!edgeCluster.second->assigned) && edgeCluster.second->clusterObjects.size() >= DE_MINOBJECTS){
				edgeCluster.second->assigned = true;
				list<EdgeCluster*> densityEdges = list<EdgeCluster*>();
				densityEdges.push_back(edgeCluster.second);
				newCanadidates.push_back(densityEdges);
			}
		}
		canadidates = newCanadidates;//���º�ѡ���м���
	}
	return resultsList;
}

int main(){
	//����·���������ͼƥ����������·�ξ���
	routeNetwork = Map(filePath, 500);//����·��
	edgeCluster();//�����ͼƥ����������·�ξ���

	//�ھ�·������
	clock_t start, finish;
	start = clock();
	resultsList = methodWithKPruningAndMoreInfo();
	finish = clock();
	cout << "��ʱ��" << finish - start << "����" << endl;

	//����·������
	cout << "���õ�" << resultsList.size() << "��ģʽ����" << endl;
	getDistinctEdges();
	filterInvalidEdgeSet();
	//getDistinctEdges();
	getTimeStatistic();
	getAverageSpeed();

	//���·������
	//outputResults("filteredResults.txt");
	return 0;
}