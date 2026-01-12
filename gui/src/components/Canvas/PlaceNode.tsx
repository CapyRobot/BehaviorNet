import { memo } from 'react';
import { Handle, Position } from '@xyflow/react';
import type { Place } from '../../store/types';
import { useNetStore } from '../../store/netStore';

interface PlaceNodeProps {
  id: string;
  data: { place: Place };
  selected?: boolean;
}

function PlaceNode({ id, data, selected }: PlaceNodeProps) {
  const { place } = data;
  const setSelection = useNetStore((s) => s.setSelection);

  const handleClick = () => {
    setSelection(id, 'place');
  };

  // Determine icon/label based on place type
  const getIcon = () => {
    switch (place.type) {
      case 'entrypoint':
        return '>';
      case 'resource_pool':
        return '#';
      case 'action':
        return 'A';
      case 'wait_with_timeout':
        return 'W';
      case 'exit_logger':
        return 'X';
      default:
        return 'P';
    }
  };

  const isAction = place.type === 'action';

  return (
    <div
      className={`place-node ${place.type} ${selected ? 'selected' : ''}`}
      onClick={handleClick}
    >
      {/* Main handles - full node coverage for flexible connections */}
      <Handle
        type="target"
        position={Position.Left}
        id="in"
        className="!bg-transparent !border-0 !w-full !h-full !left-0 !top-0 !transform-none !rounded-full"
      />
      <Handle
        type="source"
        position={Position.Right}
        id="out"
        className="!bg-transparent !border-0 !w-full !h-full !left-0 !top-0 !transform-none !rounded-full"
      />

      <div className="flex flex-col items-center text-center px-1">
        <span className="text-lg font-bold">{getIcon()}</span>
        <span className="text-xs truncate w-16">{place.label}</span>
      </div>

      {/* Action places have subplace handles for success/failure/error */}
      {isAction && (
        <div className="subplace-indicators">
          <Handle
            type="source"
            position={Position.Bottom}
            id="success"
            className="!relative !transform-none !position-static"
          >
            <div className="subplace-indicator success">S</div>
          </Handle>
          <Handle
            type="source"
            position={Position.Bottom}
            id="failure"
            className="!relative !transform-none !position-static"
          >
            <div className="subplace-indicator failure">F</div>
          </Handle>
          <Handle
            type="source"
            position={Position.Bottom}
            id="error"
            className="!relative !transform-none !position-static"
          >
            <div className="subplace-indicator error">E</div>
          </Handle>
        </div>
      )}
    </div>
  );
}

export default memo(PlaceNode);
