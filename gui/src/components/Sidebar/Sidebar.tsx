import { DragEvent } from 'react';
import type { PlaceConfigSchema } from '../../store/placeConfig';

interface Props {
  placeConfig: PlaceConfigSchema;
}

function DraggableItem({
  type,
  label,
  description,
  color,
  nodeType = 'place',
}: {
  type: string;
  label: string;
  description: string;
  color: string;
  nodeType?: 'place' | 'transition';
}) {
  const onDragStart = (event: DragEvent) => {
    event.dataTransfer.setData('application/bnet-type', type);
    event.dataTransfer.setData('application/bnet-node-type', nodeType);
    event.dataTransfer.effectAllowed = 'move';
  };

  const colorClasses: Record<string, string> = {
    green: 'border-green-500 bg-green-50 hover:bg-green-100',
    blue: 'border-blue-500 bg-blue-50 hover:bg-blue-100',
    orange: 'border-orange-500 bg-orange-50 hover:bg-orange-100',
    yellow: 'border-yellow-500 bg-yellow-50 hover:bg-yellow-100',
    red: 'border-red-500 bg-red-50 hover:bg-red-100',
    gray: 'border-gray-400 bg-gray-50 hover:bg-gray-100',
    black: 'border-gray-800 bg-gray-800 text-white hover:bg-gray-700',
  };

  return (
    <div
      draggable
      onDragStart={onDragStart}
      className={`p-3 border-2 rounded-lg cursor-grab active:cursor-grabbing transition-colors ${colorClasses[color] || colorClasses.gray}`}
    >
      <div className="font-medium text-sm">{label}</div>
      <div className="text-xs text-gray-600 mt-1">{description}</div>
    </div>
  );
}

export default function Sidebar({ placeConfig }: Props) {
  const placeTypes = Object.entries(placeConfig.placeTypes);

  return (
    <div className="w-64 bg-white border-r border-gray-200 p-4 overflow-y-auto">
      <h2 className="text-lg font-bold mb-4">Toolbox</h2>

      <div className="mb-6">
        <h3 className="text-sm font-semibold text-gray-600 mb-2 uppercase tracking-wide">
          Places
        </h3>
        <div className="space-y-2">
          {placeTypes.map(([type, def]) => (
            <DraggableItem
              key={type}
              type={type}
              label={def.label}
              description={def.description}
              color={def.color}
            />
          ))}
        </div>
      </div>

      <div>
        <h3 className="text-sm font-semibold text-gray-600 mb-2 uppercase tracking-wide">
          Transitions
        </h3>
        <div className="space-y-2">
          <DraggableItem
            type="transition"
            nodeType="transition"
            label="Transition"
            description="Connects places, fires when enabled"
            color="black"
          />
        </div>
      </div>

      <div className="mt-6 p-3 bg-blue-50 rounded-lg text-xs text-blue-800">
        <strong>Tip:</strong> Drag items to the canvas. Connect places to
        transitions by dragging from handles. Action places have S/F/E handles
        for success/failure/error outputs.
      </div>
    </div>
  );
}
