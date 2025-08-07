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
