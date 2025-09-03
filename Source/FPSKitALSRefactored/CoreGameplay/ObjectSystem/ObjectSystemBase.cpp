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

		if (TrimmedCommand.Left(2) == TEXT("//"))
		{
			continue;
		}

		FString Left, Right;  
		if (TrimmedCommand.Split(TEXT("="), &Left, &Right))  
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
			FString CommandName = *TrimmedCommand;  
			if (!CommandName.IsEmpty())  
			{  
				TArray<FString> Arguments;  
				int32 OpenParenIndex, CloseParenIndex;  
				FString ParsedCommand = TEXT("");
				if (TrimmedCommand.FindChar(TEXT('('), OpenParenIndex) && TrimmedCommand.FindChar(TEXT(')'), CloseParenIndex))  
				{  
					FString ArgumentsString = TrimmedCommand.Mid(OpenParenIndex + 1, CloseParenIndex - OpenParenIndex - 1);  
					Arguments = UKismetStringLibrary::ParseIntoArray(ArgumentsString, TEXT(","), true);  
					ParsedCommand = TrimmedCommand.Left(OpenParenIndex).TrimStartAndEnd();

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

