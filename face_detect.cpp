#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <AL/alut.h>
#include <iostream>
#include <pthread.h>

using namespace std;
using namespace cv;

#define NUM_THREADS 2


static void help()
{
    cout << "\nThis program demonstrates the cascade recognizer. Now you can use Haar or LBP features.\n"
            "This classifier can recognize many kinds of rigid objects, once the appropriate classifier is trained.\n"
            "It's most known use is for faces.\n"
            "Usage:\n"
            "./facedetect [--cascade=<cascade_path> this is the primary trained classifier such as frontal face]\n"
               "   [--nested-cascade[=nested_cascade_path this an optional secondary classifier such as eyes]]\n"
               "   [--scale=<image scale greater or equal to 1, try 1.3 for example>]\n"
               "   [--try-flip]\n"
               "   [filename|camera_index]\n\n"
            "see facedetect.cmd for one call:\n"
            "./facedetect --cascade=\"../../data/haarcascades/haarcascade_frontalface_alt.xml\" --nested-cascade=\"../../data/haarcascades/haarcascade_eye_tree_eyeglasses.xml\" --scale=1.3\n\n"
            "During execution:\n\tHit any key to quit.\n"
            "\tUsing OpenCV version " << CV_VERSION << "\n" << endl;
}

void* detectVideo( void* args );
void detectAndDraw( Mat& img, CascadeClassifier& cascade,
                    CascadeClassifier& nestedCascade,
                    double scale, bool tryflip );
void* playVideo(void* args);

string cascadeName;
string nestedCascadeName;
string videoName = "5.avi";
string audioName = "background.wav";
string warningName = "emergency.wav";
VideoCapture capture;
ALuint audioSource;
ALuint warningSource;
int speed=30;
bool flag = true;
int prev = -1;
//int countZero=0;
int countTime=0;
int countFace=0;
float averageFace=0;
int faceNum = 0;
int waterLevel=5; //1 for emergency 0 for death

int main()
{
    alutInit(0, NULL);

    pthread_t tids[NUM_THREADS]; //线程id
    pthread_create( &tids[0], NULL, detectVideo, NULL ); //参数：创建的线程id，线程参数，线程运行函数的起始地址，运行函数的参数
    pthread_create( &tids[1], NULL, playVideo, NULL );
    pthread_exit( NULL ); //等待各个线程退出后，进程才结束，否则进程强制结束，线程处于未终止的状态
    return 0;
}


void* detectVideo(void* args)
{
    VideoCapture capture(1);
    Mat frame, image;
    string inputName;
    bool tryflip;
    CascadeClassifier cascade, nestedCascade;
    double scale=1.0;

    cascadeName = "haarcascade_frontalface_default.xml";
    nestedCascadeName = "haarcascade_eye_tree_eyeglasses.xml";

    if ( !nestedCascade.load( nestedCascadeName ) )
        cerr << "WARNING: Could not load classifier cascade for nested objects" << endl;
    if( !cascade.load( cascadeName ) )
    {
        cerr << "ERROR: Could not load classifier cascade" << endl;
        help();
    }

    //capture.open(0);
    if( capture.isOpened() )
    {
        cout << "Video capturing has been started ..." << endl;

        for(;;)
        {
            capture >> frame;
            if( frame.empty() )
                break;

            Mat frame1 = frame.clone();
            detectAndDraw( frame1, cascade, nestedCascade, scale, tryflip );
            //int c =
            waitKey(10);
            /*if( c == 27 || c == 'q' || c == 'Q' )
                break;*/
        }
    }

}

void detectAndDraw( Mat& img, CascadeClassifier& cascade,
                    CascadeClassifier& nestedCascade,
                    double scale, bool tryflip )
{
    double t = 0;
    vector<Rect> faces, faces2;
    const static Scalar colors[] =
    {
        Scalar(255,0,0),
        Scalar(255,128,0),
        Scalar(255,255,0),
        Scalar(0,255,0),
        Scalar(0,128,255),
        Scalar(0,255,255),
        Scalar(0,0,255),
        Scalar(255,0,255)
    };
    Mat gray, smallImg;

    cvtColor( img, gray, COLOR_BGR2GRAY );
    double fx = 1 / scale;
    resize( gray, smallImg, Size(), fx, fx, INTER_LINEAR );
    equalizeHist( smallImg, smallImg );

    t = (double)cvGetTickCount();
    cascade.detectMultiScale( smallImg, faces,
        1.1, 2, 0
        //|CASCADE_FIND_BIGGEST_OBJECT
        //|CASCADE_DO_ROUGH_SEARCH
        |CASCADE_SCALE_IMAGE,
        Size(30, 30) );
    if( tryflip )
    {
        flip(smallImg, smallImg, 1);
        cascade.detectMultiScale( smallImg, faces2,
                                 1.1, 2, 0
                                 //|CASCADE_FIND_BIGGEST_OBJECT
                                 //|CASCADE_DO_ROUGH_SEARCH
                                 |CASCADE_SCALE_IMAGE,
                                 Size(30, 30) );
        for( vector<Rect>::const_iterator r = faces2.begin(); r != faces2.end(); r++ )
        {
            faces.push_back(Rect(smallImg.cols - r->x - r->width, r->y, r->width, r->height));
        }
    }
   // t = (double)cvGetTickCount() - t;
  //  printf( "detection time = %g ms\n", t/((double)cvGetTickFrequency()*1000.) );
    faceNum = faces.size();
    cout<<"face number: " << faceNum <<endl;
   // imshow( "result", img );
    countTime++;
    countFace = countFace + faceNum;

}

void* playVideo(void* args)
{
    Mat frame;
    capture.open(videoName);
    alGenSources(1, &audioSource);
    alSourcei(audioSource, AL_BUFFER, alutCreateBufferFromFile(audioName.c_str()));
    alSourcei(audioSource, AL_LOOPING, AL_TRUE);
    alSourcePlay(audioSource);

    alGenSources(1, &warningSource);
    alSourcei(warningSource, AL_BUFFER, alutCreateBufferFromFile(warningName.c_str()));


   // capture >> frame;

    while (1)
    {
        capture >> frame;
        if (frame.empty())
        {
            flag = true;
            averageFace = float(countFace)/countTime;
            if (averageFace > 0.8) flag = true;
            else flag = false;
            cout<<averageFace<<endl;

            averageFace = 0;
            countTime = 0;
            countFace = 0;

            string audioName;
            if (flag)
            {
                if (waterLevel == 5)  { videoName = "5m.avi"; audioName = "background.wav"; 
            		alSourceStop(audioSource);
            		alSourcei(audioSource, AL_BUFFER, alutCreateBufferFromFile(audioName.c_str()));
            		alSourcei(audioSource, AL_LOOPING, AL_TRUE);
            		alSourcePlay(audioSource);}
                else if (waterLevel == 4)  { videoName = "4m.avi"; }
                else if (waterLevel == 3)  { videoName = "3m.avi"; }
                else if (waterLevel == 2)  { videoName = "2m.avi"; }
                else if (waterLevel == 1)  { videoName = "1m.avi"; alSourcePlay(warningSource);}
                else if (waterLevel == 0)  { videoName = "0.avi"; audioName = "death.wav";     
                alSourceStop(audioSource);
            	alSourcei(audioSource, AL_BUFFER, alutCreateBufferFromFile(audioName.c_str()));
            	alSourcei(audioSource, AL_LOOPING, AL_TRUE);
            	alSourcePlay(audioSource);}
                //else  { videoName = "5m.avi"; audioName = "background.wav"; }
                if (waterLevel) waterLevel--;
                else waterLevel = 5;
            }
            else
            {
                if (waterLevel == 5 || waterLevel == 0)  
                { 
                	videoName = "5.avi"; 
                	if(audioName != "background.wav")
                	{
                		audioName = "background.wav"; 
            			alSourceStop(audioSource);
            			alSourcei(audioSource, AL_BUFFER, alutCreateBufferFromFile(audioName.c_str()));
            			alSourcei(audioSource, AL_LOOPING, AL_TRUE);
            			alSourcePlay(audioSource);
            		}
            	}
                else if (waterLevel == 4)  { videoName = "4.avi"; }
                else if (waterLevel == 3)  { videoName = "3.avi"; }
                else if (waterLevel == 2)  { videoName = "2.avi"; }
                else if (waterLevel == 1)  { videoName = "1.avi"; }
            }
            capture.open(videoName);
            cout<<videoName<<endl;
            capture >> frame;

           /* alSourceStop(audioSource);
            alSourcei(audioSource, AL_BUFFER, alutCreateBufferFromFile(audioName.c_str()));
            alSourcei(audioSource, AL_LOOPING, AL_TRUE);
            alSourcePlay(audioSource);*/
        }
        namedWindow("fish", CV_WINDOW_NORMAL);
        setWindowProperty("fish", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
        imshow("fish",frame);
        if (flag){
            if(!faceNum) speed = 250;
            else speed = 250.0/(faceNum*faceNum);
            waitKey(speed);
            float fc = faceNum*faceNum/5 +1.0;
            alSourcef(audioSource,AL_PITCH,fc);  
            
        }
        else 
        {
            waitKey(30);
             alSourcef(audioSource,AL_PITCH,1.0f);
        }  
       // if (speed) waterLevel = (waterLevel-1)%6;
    }

}
