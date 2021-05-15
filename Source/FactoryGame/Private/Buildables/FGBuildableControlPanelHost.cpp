// This file has been automatically generated by the Unreal Header Implementation tool

#include "Buildables/FGBuildableControlPanelHost.h"

AFGBuildableControlPanelHost::AFGBuildableControlPanelHost() : Super() {
	this->MaxRenderDistance = -1;
	this->mFactoryTickFunction.TickGroup = TG_PrePhysics; this->mFactoryTickFunction.EndTickGroup = TG_PrePhysics; this->mFactoryTickFunction.bTickEvenWhenPaused = false; this->mFactoryTickFunction.bCanEverTick = false; this->mFactoryTickFunction.bStartWithTickEnabled = true; this->mFactoryTickFunction.bAllowTickOnDedicatedServer = true; this->mFactoryTickFunction.TickInterval = 0;
	this->mPrimaryColor.R = -1; this->mPrimaryColor.G = -1; this->mPrimaryColor.B = -1; this->mPrimaryColor.A = 1;
	this->mSecondaryColor.R = -1; this->mSecondaryColor.G = -1; this->mSecondaryColor.B = -1; this->mSecondaryColor.A = 1;
	this->mDismantleEffectClassName = FSoftClassPath("/Game/FactoryGame/Buildable/Factory/-Shared/BP_MaterialEffect_Dismantle.BP_MaterialEffect_Dismantle_C");
	this->mBuildEffectClassName = FSoftClassPath("/Game/FactoryGame/Buildable/Factory/-Shared/BP_MaterialEffect_Build.BP_MaterialEffect_Build_C");
	this->mHighlightParticleClassName = FSoftClassPath("/Game/FactoryGame/Buildable/-Shared/Particle/NewBuildingPing.NewBuildingPing_C");
	this->SetReplicates(true);
	this->NetDormancy = DORM_Initial;
	this->NetCullDistanceSquared = 5624999936;
}
void AFGBuildableControlPanelHost::BeginPlay(){ }
TArray< AFGBuildable* >& AFGBuildableControlPanelHost::GetControlledBuildables(TSubclassOf< AFGBuildable > outputType){ return *(new TArray< AFGBuildable* >); }
void AFGBuildableControlPanelHost::OnCircuitsChanged(){ }
void AFGBuildableControlPanelHost::SearchDownstreamCircuit(){ }