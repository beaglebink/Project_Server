#include "ObjectSystemBase.h"
#include <Kismet/KismetStringLibrary.h>

TArray<FString> AObjectSystemBase::ParseCommands(const FText& InputText) const
{
	TArray<FString> Commands;

	Commands = UKismetStringLibrary::ParseIntoArray(InputText.ToString(), TEXT("\n"), true);

	return Commands;
}

void AObjectSystemBase::ParseText(const FText Text)  
{  
TArray<FString> Commands = ParseCommands(Text);  
	for (auto Command : Commands)  
	{  
		FString TrimmedCommand = Command.TrimStartAndEnd(); 
		/*
		if (TrimmedCommand.Left(2) == TEXT("#"))
		{
			continue;
		}
		*/
		int32 CommentIndex = TrimmedCommand.Find(TEXT("#"));
		if (CommentIndex != INDEX_NONE)
		{
			TrimmedCommand = TrimmedCommand.Left(CommentIndex).TrimStartAndEnd();
		}
		if (TrimmedCommand.IsEmpty())
		{
			continue;
		}

		int32 Lenght = TrimmedCommand.Len() - 1;
		bool IsFindDot = TrimmedCommand.FindChar(TEXT('.'), Lenght);

		if (!IsFindDot)
		{
			continue;
		}

		FString ObjectName = TrimmedCommand.Left(Lenght);


		if (!ObjectName.Equals(ObjectActorName, ESearchCase::CaseSensitive))
		{
			continue;
		}

		FString RestCommand = TrimmedCommand.RightChop(Lenght + 1).TrimStartAndEnd();

		FString Left, Right;
		if (RestCommand.Split(TEXT("="), &Left, &Right))
		{
			FString VariableName = *Left.TrimStartAndEnd();
			FString Value = *Right.TrimStartAndEnd();
			if (!VariableName.IsEmpty() && !Value.IsEmpty())
			{
				VariableAssignment(VariableName, Value);
			}
		}
		else
		{
			FString CommandName = *RestCommand;
			if (!CommandName.IsEmpty())
			{
				TArray<FString> Arguments;
				int32 OpenParenIndex, CloseParenIndex;
				FString ParsedCommand = TEXT("");
				if (RestCommand.FindChar(TEXT('('), OpenParenIndex) && RestCommand.FindChar(TEXT(')'), CloseParenIndex))
				{
					FString ArgumentsString = RestCommand.Mid(OpenParenIndex + 1, CloseParenIndex - OpenParenIndex - 1);
					Arguments = UKismetStringLibrary::ParseIntoArray(ArgumentsString, TEXT(","), true);
					ParsedCommand = RestCommand.Left(OpenParenIndex).TrimStartAndEnd();

					for (auto& Arg : Arguments)
					{
						Arg = Arg.TrimStartAndEnd();
					}
				}
				ExecuteCommand(ParsedCommand, Arguments);
			}
		}
	}  
}

void AObjectSystemBase::VariableAssignment_Implementation(const FString& VariableName, const FString& Value)
{

}

void AObjectSystemBase::ExecuteCommand_Implementation(const FString& Command, const TArray<FString>& Arguments)
{

}

