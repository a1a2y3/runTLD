// runTLD.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "afx.h"
#include <opencv2/opencv.hpp>
#include "tld_utils.h"
#include <iostream>
#include <sstream>
#include "TLD.h""
#include <stdio.h>
using namespace cv;
using namespace std;
//Global variables
Rect box;
bool drawing_box = false;
bool gotBB = false;
bool tl = false;
bool rep = false;
bool fromfile=false;
string video;

void readBB(char* file){
	ifstream bb_file (file);
	string line;
	getline(bb_file,line);
	istringstream linestream(line);
	string x1,y1,x2,y2;
	getline (linestream,x1, ',');
	getline (linestream,y1, ',');
	getline (linestream,x2, ',');
	getline (linestream,y2, ',');
	int x = atoi(x1.c_str());// = (int)file["bb_x"];
	int y = atoi(y1.c_str());// = (int)file["bb_y"];
	int w = atoi(x2.c_str())-x;// = (int)file["bb_w"];
	int h = atoi(y2.c_str())-y;// = (int)file["bb_h"];
	box = Rect(x,y,w,h);
}
//bounding box mouse callback
void mouseHandler(int event, int x, int y, int flags, void *param){
	switch( event ){
	case CV_EVENT_MOUSEMOVE:
		if (drawing_box){
			box.width = x-box.x;
			box.height = y-box.y;
		}
		break;
	case CV_EVENT_LBUTTONDOWN:
		drawing_box = true;
		box = Rect( x, y, 0, 0 );
		break;
	case CV_EVENT_LBUTTONUP:
		drawing_box = false;
		if( box.width < 0 ){
			box.x += box.width;
			box.width *= -1;
		}
		if( box.height < 0 ){
			box.y += box.height;
			box.height *= -1;
		}
		gotBB = true;
		break;
	}
}

void print_help(char** argv){
	printf("use:\n     %s -p /path/parameters.yml\n",argv[0]);
	printf("-s    source video\n-b        bounding box file\n-tl  track and learn\n-r     repeat\n");
}


void read_options(int argc, char** argv,VideoCapture& capture,FileStorage &fs){
	for (int i=0;i<argc;i++){
		if (strcmp(argv[i],"-b")==0){
			if (argc>i){
				readBB(argv[i+1]);
				gotBB = true;
			}
			else
				print_help(argv);
		}
		if (strcmp(argv[i],"-s")==0){
			if (argc>i){
				video = string(argv[i+1]);
				capture.open(video);
				fromfile = true;
			}
			else
				print_help(argv);

		}
		if (strcmp(argv[i],"-p")==0){
			if (argc>i){
				fs.open(argv[i+1], FileStorage::READ);
			}
			else
				print_help(argv);
		}
		if (strcmp(argv[i],"-tl")==0){
			tl = true;
		}
		if (strcmp(argv[i],"-r")==0){
			rep = true;
		}
	}
}
void read_options_cam(VideoCapture& capture,FileStorage &fs){
	gotBB = false;
	capture.open(0);
	fromfile = false;
	fs.open("..\\parameters.yml", FileStorage::READ);
	tl = true;
	rep = false;
}
void read_options_serial(string imgpath,FileStorage &fs){
	ifstream txtfile(imgpath+"init.txt");
	char rectbuff[100];
	if (txtfile)
	{
		txtfile.getline(rectbuff,50);
		sscanf(rectbuff,"%d %d %d %d",&(box.x),&(box.y),&(box.width),&(box.height));
		txtfile.close();
		cout << "initial box param" << endl;
		cout << "(x, y, width, height) = " << "(" << box.x <<", " << box.y << ", " << box.width << ", " << box.height << ")" << endl;
	}	
	//	readBB("D:\\data\\dongjing\\1\\image\\init.txt");
	gotBB = true;
	fromfile = true;
	fs.open("..\\parameters.yml", FileStorage::READ);
	tl = true;
	rep = false;
}

int main(){
	bool usecam= true;
	VideoCapture capture;
	FileStorage fs;
	string img_path;
	img_path= "D:\\data\\dongjing\\1\\image\\";
	CFileFind find;
	if (usecam)
	{
		gotBB = false;
		capture.open(0);
		//Init camera
		if (!capture.isOpened())
		{
			cout << "capture device failed to open!" << endl;
			return 1;
		}
		fromfile = false;
		fs.open("..\\parameters.yml", FileStorage::READ);
		tl = true;
		rep = true;
	}
	else
	{		
		find.FindFile( CString((img_path+"*.bmp").c_str()));
		//Read options
		read_options_serial(img_path,fs);
	}	
	//Register mouse callback to draw the bounding box
	cvNamedWindow("TLD",CV_WINDOW_AUTOSIZE);
	cvSetMouseCallback( "TLD", mouseHandler, NULL );
	//TLD framework
	TLD tld;
	//Read parameters file
	tld.read(fs.getFirstTopLevelNode());
	Mat frame;
//	Mat gray;
	Mat last_gray;
	Mat first;
	if (fromfile){
		if(!find.FindNextFile()){ 
			cout<<"读序列图错误！"; 
//			return 0;
		}
		CString filetitle = find.GetFileTitle();
		CString filepath = find.GetFilePath();
		frame = cv::imread(filepath.GetBuffer(0));
		frame.channels() == 3 ? cvtColor(frame, last_gray, CV_BGR2GRAY) : frame.copyTo(last_gray);
//		capture >> frame;
//		cvtColor(frame, last_gray, CV_RGB2GRAY);
		frame.copyTo(first);
	}else{
		capture.set(CV_CAP_PROP_FRAME_WIDTH,640);
		capture.set(CV_CAP_PROP_FRAME_HEIGHT,480);
	}

	///Initialization
GETBOUNDINGBOX:
	while(!gotBB)
	{
		if (!fromfile){
			capture >> frame;
		}
		else
			first.copyTo(frame);
		cvtColor(frame, last_gray, CV_RGB2GRAY);
		drawBox(frame,box);
		imshow("TLD", frame);
		if (cvWaitKey(33) == 'q')
			return 0;
	}
	if (min(box.width,box.height)<(int)fs.getFirstTopLevelNode()["min_win"]){
		cout << "Bounding box too small, try again." << endl;
		gotBB = false;
		goto GETBOUNDINGBOX;
	}
	//Remove callback
	cvSetMouseCallback( "TLD", NULL, NULL );
	printf("Initial Bounding Box = x:%d y:%d h:%d w:%d\n",box.x,box.y,box.width,box.height);
	//Output file
	FILE  *bb_file = fopen("bounding_boxes.txt","w");
	//TLD initialization
	tld.init(last_gray,box,bb_file);

	///Run-time
	Mat current_gray;
	BoundingBox pbox;
	vector<Point2f> pts1;
	vector<Point2f> pts2;
	bool status=true;
	int frames = 1;
	int detections = 1;
REPEAT:
	while(1){
		//get frame
		if (fromfile){
			if(!find.FindNextFile())
				break;
			CString filetitle = find.GetFileTitle();
			CString filepath = find.GetFilePath();
			frame = cv::imread(LPCSTR(filepath.GetBuffer(0)));
			
		}else{
			if(!capture.read(frame))
				break;
		}
		frame.channels() == 3 ? cvtColor(frame, current_gray, CV_BGR2GRAY) : frame.copyTo(current_gray);
		//Process Frame
		tld.processFrame(last_gray,current_gray,pts1,pts2,pbox,status,tl,bb_file);
		//Draw Points
		if (status){
			drawPoints(frame,pts1);
			drawPoints(frame,pts2,Scalar(0,255,0));
			drawBox(frame,pbox);
			detections++;
		}
		//Display
		imshow("TLD", frame);
		//swap points and images
		swap(last_gray,current_gray);
		pts1.clear();
		pts2.clear();
		frames++;
		printf("Detection rate: %d/%d\n",detections,frames);
		if (cvWaitKey(33) == 'q')
			break;
	}
	if (rep){
		rep = false;
		tl = false;
		fclose(bb_file);
		bb_file = fopen("final_detector.txt","w");
		//capture.set(CV_CAP_PROP_POS_AVI_RATIO,0);
		capture.release();
		capture.open(video);
		goto REPEAT;
	}
	fclose(bb_file);
	return 0;
}


