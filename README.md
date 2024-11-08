# Cinnamon: A Scale-Out Framework for Encrypted Computing

## Step1: Pull the container image
```
docker pull sidjay10/asplos25_cinnamon_artifact:v1
```
<!-- ## Step1: Build the container
Download the tar of the [container](), unzip it and load it
```
gunzip asplos25_cinnamon_artifact.tar.gz
docker load -i asplos25_cinnamon_artifact.tar
```
or alternatively, clone the repo and build the container
```
git clone https://
cd asplos25_cinnamon_artifact
git submodule up -->

cd docker 
docker build -t cinnamon_artifact:v1 .
```
The container image should have loaded as cinnamon_artifact:v1

## Step2: Run The Container
In your working directory, make a sub directory `outputs` and mount it to the container. This directory is where the results of the simulations will be made available. If you don't mount the directory to the container, an error message will be displayed.
```
mkdir outputs
docker run --rm -it  -v $(pwd)/outputs:/cinnamon_artifact/outputs --name cinnamon_expt sidjay10/asplos25_cinnamon_artifact:v1
```

## Step3: Build The Cinnamon Simulator
In a separate terminal window, run
```
docker exec -it cinnamon_expt ./build_cinnamon.sh
```
This command builds the cinnamon element within SST. The rest of the SST simulator is prebuilt in the container image. This command should take about 5minutes.

## Step4: Run the simulations for generating Figures 9 and 10
```
docker exec -it cinnamon_expt ./run_fig9_10.sh
```
This command should take about 20 minutes. When it completes it will produce fig9.pdf and fig10.pdf under the outputs folder

## Step5: Run the simulations for generating Figures 7, 8 and Table 2
```
docker exec -it cinnamon_expt ./run_fig7_8.sh
```
This command should take about a day. This is because it runs the simulations for long running benchmarks. When it completes it will produce fig7.pdf and fig8.pdf and table2.txt under the outputs folder.

## Step6: Stop the Container
Stop the container once experiments are completed.
```
docker stop cinnamon_expt
```