#include "RibbonLinkAdapter.h"

void URibbonLinkAdapter::UpdateRibbonLinks(const TArray<FNode>& Nodes, UNiagaraDataInterface_RibbonLinks* DataInterface)
{
    if (!DataInterface) return;

    TArray<FNiagaraRibbonLink> Converted;

    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        const FNode& Node = Nodes[i];
        for (const FNodeLink& Link : Node.Links)
        {
            if (!Nodes.IsValidIndex(Link.NeighborIndex)) continue;

            const int32 Start = Node.PositionIndex;
            const int32 End = Nodes[Link.NeighborIndex].PositionIndex;

            Converted.Add({ Start, End });
        }
    }

    DataInterface->RibbonLinks = MoveTemp(Converted);
}