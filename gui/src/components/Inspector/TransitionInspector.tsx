import { useNetStore } from '../../store/netStore';
import type { Transition } from '../../store/types';

interface Props {
  transition: Transition;
}

export default function TransitionInspector({ transition }: Props) {
  const updateTransition = useNetStore((s) => s.updateTransition);
  const deleteTransition = useNetStore((s) => s.deleteTransition);
  const actors = useNetStore((s) => s.actors);

  const handleLabelChange = (label: string) => {
    updateTransition(transition.id, { label });
  };

  const handlePriorityChange = (priority: string) => {
    const value = priority ? parseInt(priority, 10) : 1;
    updateTransition(transition.id, { priority: value });
  };

  const handleDelete = () => {
    if (confirm('Delete this transition?')) {
      deleteTransition(transition.id);
    }
  };

  const updateTokenFilter = (index: number, filter: string) => {
    const newTo = [...transition.to];
    newTo[index] = { ...newTo[index], tokenFilter: filter || undefined };
    updateTransition(transition.id, { to: newTo });
  };

  const removeOutput = (index: number) => {
    const newTo = transition.to.filter((_, i) => i !== index);
    updateTransition(transition.id, { to: newTo });
  };

  const removeInput = (placeId: string) => {
    const newFrom = transition.from.filter((f) => f !== placeId);
    updateTransition(transition.id, { from: newFrom });
  };

  return (
    <div className="space-y-4">
      <div>
        <label className="block text-sm font-medium text-gray-700 mb-1">
          ID
        </label>
        <input
          type="text"
          value={transition.id}
          disabled
          className="w-full px-3 py-2 border border-gray-300 rounded-md bg-gray-100 text-gray-500 text-sm"
        />
      </div>

      <div>
        <label className="block text-sm font-medium text-gray-700 mb-1">
          Label
        </label>
        <input
          type="text"
          value={transition.label}
          onChange={(e) => handleLabelChange(e.target.value)}
          className="w-full px-3 py-2 border border-gray-300 rounded-md text-sm focus:ring-blue-500 focus:border-blue-500"
        />
      </div>

      <div>
        <label className="block text-sm font-medium text-gray-700 mb-1">
          Priority
        </label>
        <input
          type="number"
          min="1"
          value={transition.priority || 1}
          onChange={(e) => handlePriorityChange(e.target.value)}
          className="w-full px-3 py-2 border border-gray-300 rounded-md text-sm focus:ring-blue-500 focus:border-blue-500"
        />
        <p className="text-xs text-gray-500 mt-1">
          Higher priority transitions fire first
        </p>
      </div>

      {/* Input places (from) */}
      <div>
        <label className="block text-sm font-medium text-gray-700 mb-2">
          Input Places ({transition.from.length})
        </label>
        {transition.from.length === 0 ? (
          <p className="text-xs text-gray-500 italic">
            Connect places to this transition
          </p>
        ) : (
          <div className="space-y-1">
            {transition.from.map((placeId) => (
              <div
                key={placeId}
                className="flex items-center justify-between bg-gray-50 px-2 py-1 rounded text-sm"
              >
                <span className="truncate">{placeId}</span>
                <button
                  onClick={() => removeInput(placeId)}
                  className="text-red-500 hover:text-red-700 ml-2"
                >
                  x
                </button>
              </div>
            ))}
          </div>
        )}
      </div>

      {/* Output places (to) with token filters */}
      <div>
        <label className="block text-sm font-medium text-gray-700 mb-2">
          Output Places ({transition.to.length})
        </label>
        {transition.to.length === 0 ? (
          <p className="text-xs text-gray-500 italic">
            Connect this transition to places
          </p>
        ) : (
          <div className="space-y-2">
            {transition.to.map((output, index) => (
              <div
                key={`${output.to}-${index}`}
                className="bg-gray-50 p-2 rounded"
              >
                <div className="flex items-center justify-between mb-1">
                  <span className="text-sm truncate">{output.to}</span>
                  <button
                    onClick={() => removeOutput(index)}
                    className="text-red-500 hover:text-red-700"
                  >
                    x
                  </button>
                </div>
                <div>
                  <label className="block text-xs text-gray-500 mb-1">
                    Token Filter (actor type)
                  </label>
                  <input
                    type="text"
                    value={output.tokenFilter || ''}
                    onChange={(e) => updateTokenFilter(index, e.target.value)}
                    placeholder="e.g., user::Vehicle"
                    className="w-full px-2 py-1 border border-gray-300 rounded text-xs"
                  />
                </div>
              </div>
            ))}
          </div>
        )}
        {actors.length > 0 && (
          <p className="text-xs text-gray-500 mt-1">
            Available actors: {actors.map((a) => a.id).join(', ')}
          </p>
        )}
      </div>

      <div className="pt-4 border-t">
        <button
          onClick={handleDelete}
          className="w-full px-4 py-2 bg-red-600 text-white rounded-md hover:bg-red-700 text-sm"
        >
          Delete Transition
        </button>
      </div>
    </div>
  );
}
