#include "Utility/AlsGameplayTags.h"

namespace AlsViewModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(FirstPerson, FName{ TEXTVIEW("Als.ViewMode.FirstPerson") })
		UE_DEFINE_GAMEPLAY_TAG(ThirdPerson, FName{ TEXTVIEW("Als.ViewMode.ThirdPerson") })
}

namespace AlsLocomotionModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Grounded, FName{ TEXTVIEW("Als.LocomotionMode.Grounded") })
		UE_DEFINE_GAMEPLAY_TAG(InAir, FName{ TEXTVIEW("Als.LocomotionMode.InAir") })
}

namespace AlsRotationModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(VelocityDirection, FName{ TEXTVIEW("Als.RotationMode.VelocityDirection") })
		UE_DEFINE_GAMEPLAY_TAG(ViewDirection, FName{ TEXTVIEW("Als.RotationMode.ViewDirection") })
		UE_DEFINE_GAMEPLAY_TAG(Aiming, FName{ TEXTVIEW("Als.RotationMode.Aiming") })
}

namespace AlsStanceTags
{
	UE_DEFINE_GAMEPLAY_TAG(Standing, FName{ TEXTVIEW("Als.Stance.Standing") })
		UE_DEFINE_GAMEPLAY_TAG(Crouching, FName{ TEXTVIEW("Als.Stance.Crouching") })
}

namespace AlsGaitTags
{
	UE_DEFINE_GAMEPLAY_TAG(Walking, FName{ TEXTVIEW("Als.Gait.Walking") })
		UE_DEFINE_GAMEPLAY_TAG(Running, FName{ TEXTVIEW("Als.Gait.Running") })
		UE_DEFINE_GAMEPLAY_TAG(Sprinting, FName{ TEXTVIEW("Als.Gait.Sprinting") })
}

namespace AlsOverlayModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Default, FName{ TEXTVIEW("Als.OverlayMode.Default") })
		UE_DEFINE_GAMEPLAY_TAG(Masculine, FName{ TEXTVIEW("Als.OverlayMode.Masculine") })
		UE_DEFINE_GAMEPLAY_TAG(Feminine, FName{ TEXTVIEW("Als.OverlayMode.Feminine") })
		UE_DEFINE_GAMEPLAY_TAG(Injured, FName{ TEXTVIEW("Als.OverlayMode.Injured") })
		UE_DEFINE_GAMEPLAY_TAG(HandsTied, FName{ TEXTVIEW("Als.OverlayMode.HandsTied") })
		UE_DEFINE_GAMEPLAY_TAG(M4, FName{ TEXTVIEW("Als.OverlayMode.M4") })
		UE_DEFINE_GAMEPLAY_TAG(PistolOneHanded, FName{ TEXTVIEW("Als.OverlayMode.PistolOneHanded") })
		UE_DEFINE_GAMEPLAY_TAG(PistolTwoHanded, FName{ TEXTVIEW("Als.OverlayMode.PistolTwoHanded") })
		UE_DEFINE_GAMEPLAY_TAG(Bow, FName{ TEXTVIEW("Als.OverlayMode.Bow") })
		UE_DEFINE_GAMEPLAY_TAG(Torch, FName{ TEXTVIEW("Als.OverlayMode.Torch") })
		UE_DEFINE_GAMEPLAY_TAG(Binoculars, FName{ TEXTVIEW("Als.OverlayMode.Binoculars") })
		UE_DEFINE_GAMEPLAY_TAG(Box, FName{ TEXTVIEW("Als.OverlayMode.Box") })
		UE_DEFINE_GAMEPLAY_TAG(Barrel, FName{ TEXTVIEW("Als.OverlayMode.Barrel") })
}

namespace AlsLocomotionActionTags
{
	UE_DEFINE_GAMEPLAY_TAG(Mantling, FName{ TEXTVIEW("Als.LocomotionAction.Mantling") })
		UE_DEFINE_GAMEPLAY_TAG(Ragdolling, FName{ TEXTVIEW("Als.LocomotionAction.Ragdolling") })
		UE_DEFINE_GAMEPLAY_TAG(GettingUp, FName{ TEXTVIEW("Als.LocomotionAction.GettingUp") })
		UE_DEFINE_GAMEPLAY_TAG(Rolling, FName{ TEXTVIEW("Als.LocomotionAction.Rolling") })
}

namespace AlsGroundedEntryModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(FromRoll, FName{ TEXTVIEW("Als.GroundedEntryMode.FromRoll") })
}

namespace EnemyTags
{
	UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Default, "Enemy.Default");
	UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Enemy, "Enemy");
	namespace Alien
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Alien::Red, "Enemy.Alien.Red");
	}
	namespace Baloon
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Baloon::A, "Enemy.Baloon.A");
	}
	namespace Bot
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Bot::Net, "Enemy.Bot.Net");
	}
	namespace Computer
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Computer::PossessedPink, "Enemy.Computer.PossessedPink");
	}
	namespace File
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::File::CabinetFolder, "Enemy.File.CabinetFolder");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::File::Guy, "Enemy.File.Guy");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::File::Prop, "Enemy.File.Prop");
	}
	namespace Fish
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Fish::Blob, "Enemy.Fish.Blob");
	}
	namespace Folder
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Folder::Manila, "Enemy.Folder.Manila");
	}
	namespace Furry
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Furry::Claw, "Enemy.Furry.Claw");
	}
	namespace Germ
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Germ::Red, "Enemy.Germ.Red");
	}
	namespace Ghost
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::ArmourGreen, "Enemy.Ghost.ArmourGreen");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::BloodyMouth, "Enemy.Ghost.BloodyMouth");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::BlueLegged, "Enemy.Ghost.BlueLegged");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Boolean, "Enemy.Ghost.Boolean");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Camera, "Enemy.Ghost.Camera");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Cat, "Enemy.Ghost.Cat");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Chimney, "Enemy.Ghost.Chimney");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::CreepyCute, "Enemy.Ghost.CreepyCute");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Dark, "Enemy.Ghost.Dark");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Encrypt, "Enemy.Ghost.Encrypt");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::FileBlack, "Enemy.Ghost.FileBlack");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::FleshLegged, "Enemy.Ghost.FleshLegged");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Furry, "Enemy.Ghost.Furry");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::FurryBig, "Enemy.Ghost.FurryBig");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::FurryBlue, "Enemy.Ghost.FurryBlue");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::GenericGrey, "Enemy.Ghost.GenericGrey");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::GenericGreyDark, "Enemy.Ghost.GenericGreyDark");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::GenericMetal, "Enemy.Ghost.GenericMetal");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Geometric, "Enemy.Ghost.Geometric");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::GlobRed, "Enemy.Ghost.GlobRed");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::GloopyBlue, "Enemy.Ghost.GloopyBlue");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::GoardRed, "Enemy.Ghost.GoardRed");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Gooey, "Enemy.Ghost.Gooey");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::GreenFish, "Enemy.Ghost.GreenFish");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::GreenFlesh, "Enemy.Ghost.GreenFlesh");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::GreenTeeth, "Enemy.Ghost.GreenTeeth");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::GunMetalRound, "Enemy.Ghost.GunMetalRound");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::HelmetGreen, "Enemy.Ghost.HelmetGreen");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Hex, "Enemy.Ghost.Hex");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Ink, "Enemy.Ghost.Ink");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Integer, "Enemy.Ghost.Integer");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Liquid, "Enemy.Ghost.Liquid");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Loop, "Enemy.Ghost.Loop");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::ManyEyedPink, "Enemy.Ghost.ManyEyedPink");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::MechbodyRound, "Enemy.Ghost.MechbodyRound");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::MechaClaw, "Enemy.Ghost.MechaClaw");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::MetalFleshRed, "Enemy.Ghost.MetalFleshRed");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::MohSideTeeth, "Enemy.Ghost.MohSideTeeth");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Monitor, "Enemy.Ghost.Monitor");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Newspaper, "Enemy.Ghost.Newspaper");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Nugget, "Enemy.Ghost.Nugget");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::PlantBig, "Enemy.Ghost.PlantBig");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::PlantCute, "Enemy.Ghost.PlantCute");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Pointer, "Enemy.Ghost.Pointer");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::PolygonGreen, "Enemy.Ghost.PolygonGreen");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Program, "Enemy.Ghost.Program");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::RhinoBw, "Enemy.Ghost.RhinoBw");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::RoboBlue, "Enemy.Ghost.RoboBlue");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::RoboGreen, "Enemy.Ghost.RoboGreen");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Rocket, "Enemy.Ghost.Rocket");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::RootRed, "Enemy.Ghost.RootRed");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::ScreenRed, "Enemy.Ghost.ScreenRed");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Slimy, "Enemy.Ghost.Slimy");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::SpookyCute, "Enemy.Ghost.SpookyCute");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::TentacleBeige, "Enemy.Ghost.TentacleBeige");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Termite, "Enemy.Ghost.Termite");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::ThreadBlue, "Enemy.Ghost.ThreadBlue");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::TongueSilver, "Enemy.Ghost.TongueSilver");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Tumor, "Enemy.Ghost.Tumor");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::TVHeadGrey, "Enemy.Ghost.TVHeadGrey");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::TVLeg, "Enemy.Ghost.TVLeg");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Unicode, "Enemy.Ghost.Unicode");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Ghost::Wire, "Enemy.Ghost.Wire");
	}
	namespace Golden
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Golden::Possessed, "Enemy.Golden.Possessed");
	}
	namespace Golem
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Golem::A, "Enemy.Golem.A");
	}
	namespace Gooey
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Gooey::A, "Enemy.Gooey.A");
	}
	namespace Gremlin
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Gremlin::Green, "Enemy.Gremlin.Green");
	}
	namespace Guy
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Guy::CuteYellow, "Enemy.Guy.CuteYellow");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Guy::LittleElectric, "Enemy.Guy.LittleElectric");
	}
	namespace Mine
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Mine::Code, "Enemy.Mine.Code");
	}
	namespace Mouth
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Mouth::Squiggly, "Enemy.Mouth.Squiggly");
	}
	namespace Monster
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Monster::Big, "Enemy.Monster.Big");
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Monster::Heart, "Enemy.Monster.Heart");
	}
	namespace Smoke
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Smoke::Dark, "Enemy.Smoke.Dark");
	}
	namespace Spiral
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Spiral::Green, "Enemy.Spiral.Green");
	}
	namespace Teeth
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Teeth::Side, "Enemy.Teeth.Side");
	}
	namespace Wire
	{
		UE_DEFINE_GAMEPLAY_TAG(EnemyTags::Wire::Green, "Enemy.Wire.Green");
	}
}

namespace FoodEffectTags
{
	UE_DEFINE_GAMEPLAY_TAG(Default, FName{ TEXTVIEW("Food.Effect.Default") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_1, FName{ TEXTVIEW("Food.Effect.Effect_1") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_2, FName{ TEXTVIEW("Food.Effect.Effect_2") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_3, FName{ TEXTVIEW("Food.Effect.Effect_3") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_4, FName{ TEXTVIEW("Food.Effect.Effect_4") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_5, FName{ TEXTVIEW("Food.Effect.Effect_5") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_6, FName{ TEXTVIEW("Food.Effect.Effect_6") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_7, FName{ TEXTVIEW("Food.Effect.Effect_7") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_8, FName{ TEXTVIEW("Food.Effect.Effect_8") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_9, FName{ TEXTVIEW("Food.Effect.Effect_9") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_10, FName{ TEXTVIEW("Food.Effect.Effect_10") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_11, FName{ TEXTVIEW("Food.Effect.Effect_11") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_12, FName{ TEXTVIEW("Food.Effect.Effect_12") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_13, FName{ TEXTVIEW("Food.Effect.Effect_13") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_14, FName{ TEXTVIEW("Food.Effect.Effect_14") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_15, FName{ TEXTVIEW("Food.Effect.Effect_15") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_16, FName{ TEXTVIEW("Food.Effect.Effect_16") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_17, FName{ TEXTVIEW("Food.Effect.Effect_17") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_18, FName{ TEXTVIEW("Food.Effect.Effect_18") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_19, FName{ TEXTVIEW("Food.Effect.Effect_19") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_20, FName{ TEXTVIEW("Food.Effect.Effect_20") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_21, FName{ TEXTVIEW("Food.Effect.Effect_21") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_22, FName{ TEXTVIEW("Food.Effect.Effect_22") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_23, FName{ TEXTVIEW("Food.Effect.Effect_23") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_24, FName{ TEXTVIEW("Food.Effect.Effect_24") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_25, FName{ TEXTVIEW("Food.Effect.Effect_25") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_26, FName{ TEXTVIEW("Food.Effect.Effect_26") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_27, FName{ TEXTVIEW("Food.Effect.Effect_27") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_28, FName{ TEXTVIEW("Food.Effect.Effect_28") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_29, FName{ TEXTVIEW("Food.Effect.Effect_29") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_30, FName{ TEXTVIEW("Food.Effect.Effect_30") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_31, FName{ TEXTVIEW("Food.Effect.Effect_31") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_32, FName{ TEXTVIEW("Food.Effect.Effect_32") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_33, FName{ TEXTVIEW("Food.Effect.Effect_33") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_34, FName{ TEXTVIEW("Food.Effect.Effect_34") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_35, FName{ TEXTVIEW("Food.Effect.Effect_35") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_36, FName{ TEXTVIEW("Food.Effect.Effect_36") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_37, FName{ TEXTVIEW("Food.Effect.Effect_37") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_38, FName{ TEXTVIEW("Food.Effect.Effect_38") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_39, FName{ TEXTVIEW("Food.Effect.Effect_39") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_40, FName{ TEXTVIEW("Food.Effect.Effect_40") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_41, FName{ TEXTVIEW("Food.Effect.Effect_41") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_42, FName{ TEXTVIEW("Food.Effect.Effect_42") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_43, FName{ TEXTVIEW("Food.Effect.Effect_43") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_44, FName{ TEXTVIEW("Food.Effect.Effect_44") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_45, FName{ TEXTVIEW("Food.Effect.Effect_45") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_46, FName{ TEXTVIEW("Food.Effect.Effect_46") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_47, FName{ TEXTVIEW("Food.Effect.Effect_47") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_48, FName{ TEXTVIEW("Food.Effect.Effect_48") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_49, FName{ TEXTVIEW("Food.Effect.Effect_49") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_50, FName{ TEXTVIEW("Food.Effect.Effect_50") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_51, FName{ TEXTVIEW("Food.Effect.Effect_51") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_52, FName{ TEXTVIEW("Food.Effect.Effect_52") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_53, FName{ TEXTVIEW("Food.Effect.Effect_53") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_54, FName{ TEXTVIEW("Food.Effect.Effect_54") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_55, FName{ TEXTVIEW("Food.Effect.Effect_55") })
}

namespace ClothesEffectTags
{
	UE_DEFINE_GAMEPLAY_TAG(Default, FName{ TEXTVIEW("Clothes.Effect.Default") })
		UE_DEFINE_GAMEPLAY_TAG(AlphabetCoat, FName{ TEXTVIEW("Clothes.Effect.AlphabetCoat") })
		UE_DEFINE_GAMEPLAY_TAG(ByteVest, FName{ TEXTVIEW("Clothes.Effect.ByteVest") })
		UE_DEFINE_GAMEPLAY_TAG(JanitorOveralls, FName{ TEXTVIEW("Clothes.Effect.JanitorOveralls") })
		UE_DEFINE_GAMEPLAY_TAG(WasherOveralls, FName{ TEXTVIEW("Clothes.Effect.WasherOveralls") })
		UE_DEFINE_GAMEPLAY_TAG(CalculatorGoggles, FName{ TEXTVIEW("Clothes.Effect.CalculatorGoggles") })
		UE_DEFINE_GAMEPLAY_TAG(GladiatorOutfit, FName{ TEXTVIEW("Clothes.Effect.GladiatorOutfit") })
		UE_DEFINE_GAMEPLAY_TAG(PorcupineCoat, FName{ TEXTVIEW("Clothes.Effect.PorcupineCoat") })
		UE_DEFINE_GAMEPLAY_TAG(ForcefieldCoat, FName{ TEXTVIEW("Clothes.Effect.ForcefieldCoat") })
		UE_DEFINE_GAMEPLAY_TAG(BugZapperCoat, FName{ TEXTVIEW("Clothes.Effect.BugZapperCoat") })
		UE_DEFINE_GAMEPLAY_TAG(SimmonSweatpants, FName{ TEXTVIEW("Clothes.Effect.SimmonSweatpants") })
		UE_DEFINE_GAMEPLAY_TAG(RebootVest, FName{ TEXTVIEW("Clothes.Effect.RebootVest") })
		UE_DEFINE_GAMEPLAY_TAG(NullAndVoidHat, FName{ TEXTVIEW("Clothes.Effect.NullAndVoidHat") })
		UE_DEFINE_GAMEPLAY_TAG(MagneticVest, FName{ TEXTVIEW("Clothes.Effect.MagneticVest") })
		UE_DEFINE_GAMEPLAY_TAG(AmmoBeltVest, FName{ TEXTVIEW("Clothes.Effect.AmmoBeltVest") })
		UE_DEFINE_GAMEPLAY_TAG(MasterMinMooMooSlippers, FName{ TEXTVIEW("Clothes.Effect.MasterMinMooMooSlippers") })
		UE_DEFINE_GAMEPLAY_TAG(BounceHouseSuit, FName{ TEXTVIEW("Clothes.Effect.BounceHouseSuit") })
		UE_DEFINE_GAMEPLAY_TAG(SheriffOutfit, FName{ TEXTVIEW("Clothes.Effect.SheriffOutfit") })
		UE_DEFINE_GAMEPLAY_TAG(ManillaOxfordAndSlacks, FName{ TEXTVIEW("Clothes.Effect.ManillaOxfordAndSlacks") })
		UE_DEFINE_GAMEPLAY_TAG(HoareSweaterVest, FName{ TEXTVIEW("Clothes.Effect.HoareSweaterVest") })
		UE_DEFINE_GAMEPLAY_TAG(NuttySpectacles, FName{ TEXTVIEW("Clothes.Effect.NuttySpectacles") })
		UE_DEFINE_GAMEPLAY_TAG(BugHunterUniform, FName{ TEXTVIEW("Clothes.Effect.BugHunterUniform") })
		UE_DEFINE_GAMEPLAY_TAG(SniperFocusOnLongRange, FName{ TEXTVIEW("Clothes.Effect.SniperFocusOnLongRange") })
		UE_DEFINE_GAMEPLAY_TAG(Boxer, FName{ TEXTVIEW("Clothes.Effect.Boxer") })
		UE_DEFINE_GAMEPLAY_TAG(AdminPolo, FName{ TEXTVIEW("Clothes.Effect.AdminPolo") })
		UE_DEFINE_GAMEPLAY_TAG(PorcelainCannon, FName{ TEXTVIEW("Clothes.Effect.PorcelainCannon") })
		UE_DEFINE_GAMEPLAY_TAG(CarlOvercoat, FName{ TEXTVIEW("Clothes.Effect.CarlOvercoat") })
		UE_DEFINE_GAMEPLAY_TAG(DebsFootballPads, FName{ TEXTVIEW("Clothes.Effect.DebsFootballPads") })
		UE_DEFINE_GAMEPLAY_TAG(SimonSweater, FName{ TEXTVIEW("Clothes.Effect.SimonSweater") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_29, FName{ TEXTVIEW("Clothes.Effect.Effect_29") })
		UE_DEFINE_GAMEPLAY_TAG(LaoEddieNightRobe, FName{ TEXTVIEW("Clothes.Effect.LaoEddieNightRobe") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_31, FName{ TEXTVIEW("Clothes.Effect.Effect_31") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_32, FName{ TEXTVIEW("Clothes.Effect.Effect_32") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_33, FName{ TEXTVIEW("Clothes.Effect.Effect_33") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_34, FName{ TEXTVIEW("Clothes.Effect.Effect_34") })
		UE_DEFINE_GAMEPLAY_TAG(DesperadoPoncho, FName{ TEXTVIEW("Clothes.Effect.DesperadoPoncho") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_36, FName{ TEXTVIEW("Clothes.Effect.Effect_36") })
		UE_DEFINE_GAMEPLAY_TAG(WW2Uniform, FName{ TEXTVIEW("Clothes.Effect.WW2Uniform") })
		UE_DEFINE_GAMEPLAY_TAG(GreenhouseOutfit, FName{ TEXTVIEW("Clothes.Effect.GreenhouseOutfit") })
		UE_DEFINE_GAMEPLAY_TAG(HeartShapedSweater, FName{ TEXTVIEW("Clothes.Effect.HeartShapedSweater") })
		UE_DEFINE_GAMEPLAY_TAG(CableRepairOutfit, FName{ TEXTVIEW("Clothes.Effect.CableRepairOutfit") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_41, FName{ TEXTVIEW("Clothes.Effect.Effect_41") })
		UE_DEFINE_GAMEPLAY_TAG(TroubleshooterJacket, FName{ TEXTVIEW("Clothes.Effect.TroubleshooterJacket") })
		UE_DEFINE_GAMEPLAY_TAG(HotSwapPatch, FName{ TEXTVIEW("Clothes.Effect.HotSwapPatch") })
		UE_DEFINE_GAMEPLAY_TAG(ChefApron, FName{ TEXTVIEW("Clothes.Effect.ChefApron") })
		UE_DEFINE_GAMEPLAY_TAG(MiddleAgedCyborgSamuraiTortoiseShell, FName{ TEXTVIEW("Clothes.Effect.MiddleAgedCyborgSamuraiTortoiseShell") })
		UE_DEFINE_GAMEPLAY_TAG(RastaRobe, FName{ TEXTVIEW("Clothes.Effect.RastaRobe") })
		UE_DEFINE_GAMEPLAY_TAG(UndertakerCloak, FName{ TEXTVIEW("Clothes.Effect.UndertakerCloak") })
		UE_DEFINE_GAMEPLAY_TAG(TranquilBlouse, FName{ TEXTVIEW("Clothes.Effect.TranquilBlouse") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_49, FName{ TEXTVIEW("Clothes.Effect.Effect_49") })
		UE_DEFINE_GAMEPLAY_TAG(KnuthOvercoat, FName{ TEXTVIEW("Clothes.Effect.KnuthOvercoat") })
		UE_DEFINE_GAMEPLAY_TAG(VcarSweatShirt, FName{ TEXTVIEW("Clothes.Effect.VcarSweatShirt") })
		UE_DEFINE_GAMEPLAY_TAG(Effect_52, FName{ TEXTVIEW("Clothes.Effect.Effect_52") })
}
