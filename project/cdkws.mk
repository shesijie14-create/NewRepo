.PHONY: clean All Project_Title Project_PreBuild Project_Build Project_PostBuild

All: Project_Title Project_PreBuild Project_Build Project_PostBuild

Project_Title:
	@echo "----------Building project:[ txw82xApp - FLASH ]----------"

Project_PreBuild:
	@echo Executing Pre Build commands ...
	@export CDKPath="D:/C-Sky/CDK" CDK_VERSION="V2.24.14" CPU="E804DF" ProjectName="txw82xApp" ProjectPath="E:/clash/tijiao/txw826/TXW82x_FPV-v2.7.0.7-39347_wpa3/project/" && E:/clash/tijiao/txw826/TXW82x_FPV-v2.7.0.7-39347_wpa3/project/prebuild.sh $<
	@echo Done

Project_Build:
	@make -r -f txw82xApp.mk -j 8 -C  ./ 

Project_PostBuild:
	@echo Executing Post Build commands ...
	@export CDKPath="D:/C-Sky/CDK" CDK_VERSION="V2.24.14" CPU="E804DF" ProjectName="txw82xApp" ProjectPath="E:/clash/tijiao/txw826/TXW82x_FPV-v2.7.0.7-39347_wpa3/project/" && E:/clash/tijiao/txw826/TXW82x_FPV-v2.7.0.7-39347_wpa3/project/BuildBIN.sh
	@echo Done


clean:
	@echo "----------Cleaning project:[ txw82xApp - FLASH ]----------"

