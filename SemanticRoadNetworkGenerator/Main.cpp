#include<iostream>
#include<iterator>
#include<string>
#include"../MapLibraries/Map.h"
using namespace std;

string rootDirectory = "D:\\Document\\MDM Lab\\Data\\";
string mapDirectory = "�¼��¹켣����\\";
string poiFilePath = "NDBC��չ\\poi.csv";
string semanticRoadFilePath = "semanticRoad.txt";
double neighborRange = 200.0;
Map routeNetwork(rootDirectory + mapDirectory, 500);
map<string, int> categories;


void generateSemanticRouteNetwork() {
	ifstream fin(rootDirectory + poiFilePath);
	double lat, lon;
	string category;
	char separator;
	fin >> category;
	int last = 0;
	int categoryIndex = 0;
	for (int i = 0; i < category.size(); ++i) {
		if (category[i] == ',') {
			categories.insert(make_pair(category.substr(last, i - last), categoryIndex++));
			last = i + 1;
		}
	}
	categories.insert(make_pair(category.substr(last, category.size() - last), categoryIndex++));
	while (fin >> lat >> separator >> lon >> separator >> category) {
		vector<Edge*> dest;
		routeNetwork.getNearEdges(lat, lon, neighborRange, dest);
		for each (Edge* edgePtr in dest)
		{
			if (edgePtr->poiNums.size() == 0) {
				edgePtr->poiNums = vector<double>(categories.size());
			}
			++edgePtr->poiNums[categories[category]];
		}
	}
	fin.close();
}

void outputSemanticRouteNetwork(string filePath) {
	ofstream fout(filePath);
	bool first = true;
	for each (pair<string, int> category in categories)
	{
		if (first) {
			first = false;
		}
		else {
			fout << ",";
		}
		fout << category.first;
	}
	fout << endl;
	for each(Edge* edgePtr in routeNetwork.edges) {
		if (edgePtr == NULL) continue;
		fout << edgePtr->id;
		for each(double num in edgePtr->poiNums) {
			fout << "," << num;
		}
		fout << endl;
	}
	fout.close();
}

void main() {
	generateSemanticRouteNetwork();
	outputSemanticRouteNetwork(semanticRoadFilePath);
}