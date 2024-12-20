# Cinnamon: A Scale-Out Framework for Encrypted AI

## Step1: Pull the container image
```
docker pull sidjay10/asplos25_cinnamon_artifact:v1
```

## Step2: Run The Container
In your working directory, make a sub directory `outputs` and mount it to the container. This directory is where the results of the simulations will be made available. If you don't mount the directory to the container, an error message will be displayed.
```
mkdir -p asplos25_cinnamon_artifact/outputs
cd asplos25_cinnamon_artifact
docker run --rm -it  -v $(pwd)/outputs:/cinnamon_artifact/outputs --name cinnamon sidjay10/asplos25_cinnamon_artifact:v1
```

## Step3: Build The Cinnamon Simulator
In a separate terminal window, run
```
docker exec -it cinnamon ./build_cinnamon.sh
```
This command builds the cinnamon element within SST. The rest of the SST simulator is prebuilt in the container image. This command should take about 5minutes.

## Step4: Run the simulations for generating Figures 
```
docker exec -it cinnamon ./run_keyswitch_comparison.sh
docker exec -it cinnamon ./run_bootstrap_comparison.sh
```
These commands should take about 20 minutes. When it completes it will produce keyswitch\_comparison.pdf and bootstrap\_comparison.pdf under the outputs folder

```
docker exec -it cinnamon ./run_performance.sh
```
This command should take about a day. This is because it runs the simulations for long running benchmarks. When it completes it will produce performance.pdf and performance\_per\_dollar.pdf and performance\_table.txt under the outputs folder.

## Step6: Stop the Container
Stop the container once experiments are completed.
```
docker stop cinnamon
```
