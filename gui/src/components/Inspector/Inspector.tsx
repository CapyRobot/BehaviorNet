import { useNetStore } from '../../store/netStore';
import PlaceInspector from './PlaceInspector';
import TransitionInspector from './TransitionInspector';

export default function Inspector() {
  const selectedId = useNetStore((s) => s.selectedId);
  const selectedType = useNetStore((s) => s.selectedType);
  const places = useNetStore((s) => s.places);
  const transitions = useNetStore((s) => s.transitions);

  if (!selectedId || !selectedType) {
    return (
      <div className="w-72 bg-white border-l border-gray-200 p-4">
        <h2 className="text-lg font-bold mb-4">Inspector</h2>
        <p className="text-sm text-gray-500">
          Select a place or transition to edit its properties.
        </p>
      </div>
    );
  }

  if (selectedType === 'place') {
    const place = places.find((p) => p.id === selectedId);
    if (!place) {
      return (
        <div className="w-72 bg-white border-l border-gray-200 p-4">
          <h2 className="text-lg font-bold mb-4">Inspector</h2>
          <p className="text-sm text-red-500">Place not found</p>
        </div>
      );
    }
    return (
      <div className="w-72 bg-white border-l border-gray-200 p-4 overflow-y-auto">
        <h2 className="text-lg font-bold mb-4">Place</h2>
        <PlaceInspector place={place} />
      </div>
    );
  }

  if (selectedType === 'transition') {
    const transition = transitions.find((t) => t.id === selectedId);
    if (!transition) {
      return (
        <div className="w-72 bg-white border-l border-gray-200 p-4">
          <h2 className="text-lg font-bold mb-4">Inspector</h2>
          <p className="text-sm text-red-500">Transition not found</p>
        </div>
      );
    }
    return (
      <div className="w-72 bg-white border-l border-gray-200 p-4 overflow-y-auto">
        <h2 className="text-lg font-bold mb-4">Transition</h2>
        <TransitionInspector transition={transition} />
      </div>
    );
  }

  return null;
}
