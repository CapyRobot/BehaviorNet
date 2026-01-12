import { create } from 'zustand';
import type { Node, Edge } from '@xyflow/react';
import {
  Place,
  Transition,
  ActorDefinition,
  ActionDefinition,
  PlaceType,
  GlobalErrorHandler,
  BNetConfig,
  getDefaultParams,
} from './types';

// Generate unique IDs
let placeCounter = 1;
let transitionCounter = 1;

function generatePlaceId(): string {
  return `place_${placeCounter++}`;
}

function generateTransitionId(): string {
  return `transition_${transitionCounter++}`;
}

// Convert our data model to React Flow nodes/edges
export function placesToNodes(places: Place[]): Node[] {
  return places.map((place) => ({
    id: place.id,
    type: 'place',
    position: place.position,
    data: { place },
  }));
}

export function transitionsToNodes(transitions: Transition[]): Node[] {
  return transitions.map((transition) => ({
    id: transition.id,
    type: 'transition',
    position: transition.position,
    data: { transition },
  }));
}

export function connectionsToEdges(transitions: Transition[]): Edge[] {
  const edges: Edge[] = [];

  transitions.forEach((transition) => {
    // Input arcs: place -> transition
    transition.from.forEach((fromPlaceId) => {
      // Handle subplace IDs like "place_1::success"
      let source = fromPlaceId;
      let sourceHandle: string | undefined = undefined;

      if (fromPlaceId.includes('::')) {
        const [placeId, subplace] = fromPlaceId.split('::');
        source = placeId;
        sourceHandle = subplace;
      }

      edges.push({
        id: `${fromPlaceId}->${transition.id}`,
        source,
        sourceHandle,
        target: transition.id,
        targetHandle: 'in',
      });
    });

    // Output arcs: transition -> place
    transition.to.forEach((output) => {
      edges.push({
        id: `${transition.id}->${output.to}`,
        source: transition.id,
        sourceHandle: 'out',
        target: output.to,
        targetHandle: 'in',
        label: output.tokenFilter || undefined,
      });
    });
  });

  return edges;
}

interface NetState {
  // Data
  places: Place[];
  transitions: Transition[];
  actors: ActorDefinition[];
  actions: ActionDefinition[];
  globalErrorHandler?: GlobalErrorHandler;

  // UI state
  selectedId: string | null;
  selectedType: 'place' | 'transition' | null;

  // Place operations
  addPlace: (type: PlaceType, position: { x: number; y: number }) => string;
  updatePlace: (id: string, updates: Partial<Place>) => void;
  deletePlace: (id: string) => void;

  // Transition operations
  addTransition: (position: { x: number; y: number }) => string;
  updateTransition: (id: string, updates: Partial<Transition>) => void;
  deleteTransition: (id: string) => void;

  // Connection operations
  addConnection: (sourceId: string, targetId: string) => void;
  removeConnection: (sourceId: string, targetId: string) => void;

  // Actor/Action registry
  addActor: (actor: ActorDefinition) => void;
  updateActor: (id: string, updates: Partial<ActorDefinition>) => void;
  deleteActor: (id: string) => void;
  addAction: (action: ActionDefinition) => void;
  updateAction: (id: string, updates: Partial<ActionDefinition>) => void;
  deleteAction: (id: string) => void;

  // Selection
  setSelection: (id: string | null, type: 'place' | 'transition' | null) => void;

  // Position updates (for drag)
  updateNodePosition: (id: string, position: { x: number; y: number }) => void;

  // Import/Export
  exportConfig: () => BNetConfig;
  importConfig: (config: BNetConfig) => void;
  clearAll: () => void;
}

export const useNetStore = create<NetState>((set, get) => ({
  // Initial state
  places: [],
  transitions: [],
  actors: [],
  actions: [],
  globalErrorHandler: undefined,
  selectedId: null,
  selectedType: null,

  // Place operations
  addPlace: (type, position) => {
    const id = generatePlaceId();
    const newPlace: Place = {
      id,
      label: type.charAt(0).toUpperCase() + type.slice(1).replace(/_/g, ' '),
      type,
      params: getDefaultParams(type),
      position,
    };
    set((state) => ({ places: [...state.places, newPlace] }));
    return id;
  },

  updatePlace: (id, updates) => {
    set((state) => ({
      places: state.places.map((p) =>
        p.id === id ? { ...p, ...updates } : p
      ),
    }));
  },

  deletePlace: (id) => {
    set((state) => ({
      places: state.places.filter((p) => p.id !== id),
      // Also remove any transitions connected to this place
      transitions: state.transitions.map((t) => ({
        ...t,
        from: t.from.filter((f) => !f.startsWith(id)),
        to: t.to.filter((o) => !o.to.startsWith(id)),
      })).filter((t) => t.from.length > 0 || t.to.length > 0),
      selectedId: state.selectedId === id ? null : state.selectedId,
      selectedType: state.selectedId === id ? null : state.selectedType,
    }));
  },

  // Transition operations
  addTransition: (position) => {
    const id = generateTransitionId();
    const newTransition: Transition = {
      id,
      label: 'T',
      from: [],
      to: [],
      position,
      priority: 1,
    };
    set((state) => ({ transitions: [...state.transitions, newTransition] }));
    return id;
  },

  updateTransition: (id, updates) => {
    set((state) => ({
      transitions: state.transitions.map((t) =>
        t.id === id ? { ...t, ...updates } : t
      ),
    }));
  },

  deleteTransition: (id) => {
    set((state) => ({
      transitions: state.transitions.filter((t) => t.id !== id),
      selectedId: state.selectedId === id ? null : state.selectedId,
      selectedType: state.selectedId === id ? null : state.selectedType,
    }));
  },

  // Connection operations
  addConnection: (sourceId, targetId) => {
    const { places } = get();

    // Determine if source is a place or transition
    const sourceIsPlace = places.some((p) => p.id === sourceId) ||
      sourceId.includes('::'); // Subplace like "action::success"
    const targetIsPlace = places.some((p) => p.id === targetId);

    if (sourceIsPlace && !targetIsPlace) {
      // Place -> Transition: add to transition's "from"
      set((state) => ({
        transitions: state.transitions.map((t) =>
          t.id === targetId && !t.from.includes(sourceId)
            ? { ...t, from: [...t.from, sourceId] }
            : t
        ),
      }));
    } else if (!sourceIsPlace && targetIsPlace) {
      // Transition -> Place: add to transition's "to"
      set((state) => ({
        transitions: state.transitions.map((t) =>
          t.id === sourceId && !t.to.some((o) => o.to === targetId)
            ? { ...t, to: [...t.to, { to: targetId }] }
            : t
        ),
      }));
    }
    // Place -> Place or Transition -> Transition connections are invalid
  },

  removeConnection: (sourceId, targetId) => {
    const { places } = get();
    const sourceIsPlace = places.some((p) => p.id === sourceId) ||
      sourceId.includes('::');

    if (sourceIsPlace) {
      // Remove from transition's "from"
      set((state) => ({
        transitions: state.transitions.map((t) =>
          t.id === targetId
            ? { ...t, from: t.from.filter((f) => f !== sourceId) }
            : t
        ),
      }));
    } else {
      // Remove from transition's "to"
      set((state) => ({
        transitions: state.transitions.map((t) =>
          t.id === sourceId
            ? { ...t, to: t.to.filter((o) => o.to !== targetId) }
            : t
        ),
      }));
    }
  },

  // Actor/Action registry
  addActor: (actor) => {
    set((state) => ({ actors: [...state.actors, actor] }));
  },

  updateActor: (id, updates) => {
    set((state) => ({
      actors: state.actors.map((a) =>
        a.id === id ? { ...a, ...updates } : a
      ),
    }));
  },

  deleteActor: (id) => {
    set((state) => ({
      actors: state.actors.filter((a) => a.id !== id),
    }));
  },

  addAction: (action) => {
    set((state) => ({ actions: [...state.actions, action] }));
  },

  updateAction: (id, updates) => {
    set((state) => ({
      actions: state.actions.map((a) =>
        a.id === id ? { ...a, ...updates } : a
      ),
    }));
  },

  deleteAction: (id) => {
    set((state) => ({
      actions: state.actions.filter((a) => a.id !== id),
    }));
  },

  // Selection
  setSelection: (id, type) => {
    set({ selectedId: id, selectedType: type });
  },

  // Position updates
  updateNodePosition: (id, position) => {
    const { places, transitions } = get();

    if (places.some((p) => p.id === id)) {
      set((state) => ({
        places: state.places.map((p) =>
          p.id === id ? { ...p, position } : p
        ),
      }));
    } else if (transitions.some((t) => t.id === id)) {
      set((state) => ({
        transitions: state.transitions.map((t) =>
          t.id === id ? { ...t, position } : t
        ),
      }));
    }
  },

  // Export
  exportConfig: () => {
    const { places, transitions, actors, actions, globalErrorHandler } = get();
    return {
      actors,
      actions,
      places,
      transitions,
      globalErrorHandler,
    };
  },

  // Import
  importConfig: (config) => {
    // Update counters based on imported IDs
    const maxPlaceNum = Math.max(
      0,
      ...config.places.map((p) => {
        const match = p.id.match(/place_(\d+)/);
        return match ? parseInt(match[1], 10) : 0;
      })
    );
    const maxTransitionNum = Math.max(
      0,
      ...config.transitions.map((t) => {
        const match = t.id.match(/transition_(\d+)/);
        return match ? parseInt(match[1], 10) : 0;
      })
    );
    placeCounter = maxPlaceNum + 1;
    transitionCounter = maxTransitionNum + 1;

    set({
      places: config.places,
      transitions: config.transitions,
      actors: config.actors,
      actions: config.actions,
      globalErrorHandler: config.globalErrorHandler,
      selectedId: null,
      selectedType: null,
    });
  },

  clearAll: () => {
    placeCounter = 1;
    transitionCounter = 1;
    set({
      places: [],
      transitions: [],
      actors: [],
      actions: [],
      globalErrorHandler: undefined,
      selectedId: null,
      selectedType: null,
    });
  },
}));
