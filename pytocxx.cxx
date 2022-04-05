#include <iostream>
#include <vector>
#include "Fir1.h"
#include "audioio.h"
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <numeric>
#include <limits>
#include <iomanip>
std::vector<double> trumpet_values;
std::vector<double> vibraphone_values;

AudioReader arT("Trumpet.wav");
AudioReader arV("Vibraphone.wav");

int sr = 96000;
int trial = 2*sr;
double fl = 0.3;
double ss = 4.9;
double lrate = 0.002;
int nc = int(fl*sr);
int ns = int(sr*ss);

int fnum= 1;

int main(){
Fir1 fir(nc);
fir.setLearningRate(lrate);
//fill vectors 
while (!arT.eof())
    trumpet_values.push_back(arT.get());
while (!arV.eof())
    vibraphone_values.push_back(arV.get());
auto wet_max =*std::max_element(std::begin(vibraphone_values), std::end(vibraphone_values));
auto wet_min =*std::min_element(std::begin(vibraphone_values), std::end(vibraphone_values));
auto dry_max=*std::max_element(std::begin(trumpet_values), std::end(trumpet_values));
auto dry_min=*std::min_element(std::begin(trumpet_values), std::end(trumpet_values));

std::cout<<std::fixed<<std::setprecision(11);
std::cout << "Wet file. Samples: "<< vibraphone_values.size() <<", max/min normalised samples: " << wet_max<<", " << wet_min << std::endl;
std::cout << "Dry file. Samples: "<< vibraphone_values.size() <<", max/min normalised samples: " << dry_max<<", " << dry_min << std::endl;

std::vector<double> training ={ trumpet_values.begin()+ns,trumpet_values.end()}; //start at later value
std::vector<double> src ={vibraphone_values.begin()+ns,vibraphone_values.end()}; 
std::vector<double> src_squared(src.size());
std::transform(src.begin(),src.end(),src_squared.begin(),[](double c){return c*c;}); //square each element

double sum = std::accumulate(src_squared.begin(), src_squared.end(), 0.0);
double msi = sum/src_squared.size();


if (training.size()!=src.size()){
throw std::invalid_argument( "interference and source samples are different lengths" );
}
std::cout << "Pre-training..." << std::endl;
int length;

if (trial < src.size()){
	length = trial;
}else{
	length = src.size();
}

for (int i = 0;i<length;i++){
	fir.lms_update(src[i]-fir.filter(training[i]));
} 
std::cout << "Processing..." << std::endl;
std::vector<double> error(training.size(),0.0);
for (int i = 0;i<training.size();i++){
	error[i]=src[i]-fir.filter(training[i]);
	fir.lms_update(error[i]);
}


std::vector<double> error_squared(training.size());
std::transform(error.begin(),error.end(),error_squared.begin(),[](double c){return c*c;});
double error_sum = std::accumulate(error_squared.begin(), error_squared.end(), 0.0);
double powg=(error_sum/error_squared.size())/msi;

std::cout << "Power gain: " << powg << std::endl;
std::string cmd("sox -t dat - -b 24 -e signed-integer result.wav");
FILE* pipe = popen(cmd.c_str(),"w");
fprintf(pipe, "; Sample Rate 96000\n; Channels 1\n");
for (int i = 0;i<error.size();i++){
	fprintf(pipe, "\t%f\t %f\n", float(i)/96000, error[i]);
}
pclose(pipe);
}
