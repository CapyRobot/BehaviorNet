import { useCallback, useMemo, DragEvent } from 'react';
import {
  ReactFlow,
  Background,
  Controls,
  MiniMap,
  Connection,
  NodeChange,
  BackgroundVariant,
  MarkerType,
  ConnectionMode,
  type Node,
  type Edge,
} from '@xyflow/react';
import '@xyflow/react/dist/style.css';

import PlaceNode from './PlaceNode';
import TransitionNode from './TransitionNode';
import {
  useNetStore,
  placesToNodes,
  transitionsToNodes,
  connectionsToEdges,
} from '../../store/netStore';
import type { PlaceType, Place } from '../../store/types';

const nodeTypes = {
  place: PlaceNode,
  transition: TransitionNode,
};

// Default edge options with arrow
const defaultEdgeOptions = {
  markerEnd: {
    type: MarkerType.ArrowClosed,
    width: 20,
    height: 20,
  },
  style: {
    strokeWidth: 2,
  },
};

export default function NetCanvas() {
  const places = useNetStore((s) => s.places);
  const transitions = useNetStore((s) => s.transitions);
  const addPlace = useNetStore((s) => s.addPlace);
  const addTransition = useNetStore((s) => s.addTransition);
  const addConnection = useNetStore((s) => s.addConnection);
  const updateNodePosition = useNetStore((s) => s.updateNodePosition);
  const setSelection = useNetStore((s) => s.setSelection);
  const selectedId = useNetStore((s) => s.selectedId);

  // Convert store data to React Flow format
  const nodes = useMemo(() => {
    const placeNodes = placesToNodes(places);
    const transitionNodes = transitionsToNodes(transitions);
    return [...placeNodes, ...transitionNodes].map((node) => ({
      ...node,
      selected: node.id === selectedId,
    }));
  }, [places, transitions, selectedId]);

  const edges = useMemo(() => {
    const rawEdges = connectionsToEdges(transitions);
    // Add marker to all edges
    return rawEdges.map((edge: Edge) => ({
      ...edge,
      markerEnd: {
        type: MarkerType.ArrowClosed,
        width: 15,
        height: 15,
      },
    }));
  }, [transitions]);

  // Handle node position changes (drag)
  const onNodesChange = useCallback(
    (changes: NodeChange[]) => {
      changes.forEach((change) => {
        if (change.type === 'position' && 'position' in change && change.position) {
          updateNodePosition(change.id, change.position);
        }
      });
    },
    [updateNodePosition]
  );

  // Handle new connections - determine direction based on node types
  const onConnect = useCallback(
    (connection: Connection) => {
      if (!connection.source || !connection.target) return;

      const sourceNode = nodes.find((n) => n.id === connection.source);
      const targetNode = nodes.find((n) => n.id === connection.target);

      if (!sourceNode || !targetNode) return;

      // Determine proper source/target based on node types
      // Valid connections: place -> transition, transition -> place
      // (including subplaces like action::success -> transition)

      let sourceId = connection.source;
      let targetId = connection.target;

      // If source is place and target is place, or source is transition and target is transition, swap if needed
      const sourceIsPlace = sourceNode.type === 'place';
      const targetIsPlace = targetNode.type === 'place';

      if (sourceIsPlace && targetIsPlace) {
        // Invalid: place -> place
        return;
      }

      if (!sourceIsPlace && !targetIsPlace) {
        // Invalid: transition -> transition
        return;
      }

      // Handle subplace connections (from action place's success/failure/error handles)
      if (sourceIsPlace && connection.sourceHandle &&
          connection.sourceHandle !== 'in' && connection.sourceHandle !== 'out') {
        sourceId = `${connection.source}::${connection.sourceHandle}`;
      }

      // Add the connection in the correct direction
      addConnection(sourceId, targetId);
    },
    [nodes, addConnection]
  );

  // Handle drop from sidebar
  const onDragOver = useCallback((event: DragEvent) => {
    event.preventDefault();
    event.dataTransfer.dropEffect = 'move';
  }, []);

  const onDrop = useCallback(
    (event: DragEvent) => {
      event.preventDefault();

      const type = event.dataTransfer.getData('application/bnet-type');
      const nodeType = event.dataTransfer.getData('application/bnet-node-type');

      if (!type) return;

      // Get the position relative to the React Flow canvas
      const reactFlowBounds = event.currentTarget.getBoundingClientRect();
      const position = {
        x: event.clientX - reactFlowBounds.left,
        y: event.clientY - reactFlowBounds.top,
      };

      if (nodeType === 'transition') {
        addTransition(position);
      } else {
        addPlace(type as PlaceType, position);
      }
    },
    [addPlace, addTransition]
  );

  // Deselect when clicking on canvas background
  const onPaneClick = useCallback(() => {
    setSelection(null, null);
  }, [setSelection]);

  return (
    <div className="flex-1 h-full">
      <ReactFlow
        nodes={nodes}
        edges={edges}
        onNodesChange={onNodesChange}
        onConnect={onConnect}
        onDragOver={onDragOver}
        onDrop={onDrop}
        onPaneClick={onPaneClick}
        nodeTypes={nodeTypes}
        defaultEdgeOptions={defaultEdgeOptions}
        fitView
        snapToGrid
        snapGrid={[15, 15]}
        connectionMode={ConnectionMode.Loose}
      >
        <Background variant={BackgroundVariant.Dots} gap={15} size={1} />
        <Controls />
        <MiniMap
          nodeColor={(node: Node) => {
            if (node.type === 'transition') return '#000000';
            const place = (node.data as { place?: Place })?.place;
            if (!place) return '#9CA3AF';
            switch (place.type) {
              case 'entrypoint':
                return '#22C55E';
              case 'resource_pool':
                return '#3B82F6';
              case 'action':
                return '#F97316';
              case 'wait_with_timeout':
                return '#EAB308';
              case 'exit_logger':
                return '#EF4444';
              default:
                return '#9CA3AF';
            }
          }}
        />
      </ReactFlow>
    </div>
  );
}
