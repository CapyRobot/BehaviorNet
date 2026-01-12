import { memo } from 'react';
import { Handle, Position } from '@xyflow/react';
import type { Transition } from '../../store/types';
import { useNetStore } from '../../store/netStore';

interface TransitionNodeProps {
  id: string;
  data: { transition: Transition };
  selected?: boolean;
}

function TransitionNode({ id, data, selected }: TransitionNodeProps) {
  const { transition } = data;
  const setSelection = useNetStore((s) => s.setSelection);

  const handleClick = () => {
    setSelection(id, 'transition');
  };

  return (
    <div
      className={`transition-node ${selected ? 'selected' : ''}`}
      onClick={handleClick}
      title={`${transition.label} (Priority: ${transition.priority || 1})`}
    >
      {/* Single handle that works for both directions */}
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
    </div>
  );
}

export default memo(TransitionNode);
