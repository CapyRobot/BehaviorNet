import { useNetStore } from '../../store/netStore';
import type {
  Place,
  EntrypointParams,
  ResourcePoolParams,
  ActionParams,
  WaitWithTimeoutParams,
} from '../../store/types';

interface Props {
  place: Place;
}

export default function PlaceInspector({ place }: Props) {
  const updatePlace = useNetStore((s) => s.updatePlace);
  const deletePlace = useNetStore((s) => s.deletePlace);
  const actors = useNetStore((s) => s.actors);
  const actions = useNetStore((s) => s.actions);

  const handleLabelChange = (label: string) => {
    updatePlace(place.id, { label });
  };

  const handleCapacityChange = (capacity: string) => {
    const value = capacity ? parseInt(capacity, 10) : undefined;
    updatePlace(place.id, { tokenCapacity: value });
  };

  const handleDelete = () => {
    if (confirm('Delete this place?')) {
      deletePlace(place.id);
    }
  };

  // Type-specific parameter updates
  const updateParams = <T extends Place['params']>(
    updates: Partial<T['config']>
  ) => {
    updatePlace(place.id, {
      params: {
        ...place.params,
        config: { ...place.params.config, ...updates },
      } as Place['params'],
    });
  };

  return (
    <div className="space-y-4">
      <div>
        <label className="block text-sm font-medium text-gray-700 mb-1">
          ID
        </label>
        <input
          type="text"
          value={place.id}
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
          value={place.label}
          onChange={(e) => handleLabelChange(e.target.value)}
          className="w-full px-3 py-2 border border-gray-300 rounded-md text-sm focus:ring-blue-500 focus:border-blue-500"
        />
      </div>

      <div>
        <label className="block text-sm font-medium text-gray-700 mb-1">
          Type
        </label>
        <input
          type="text"
          value={place.type}
          disabled
          className="w-full px-3 py-2 border border-gray-300 rounded-md bg-gray-100 text-gray-500 text-sm capitalize"
        />
      </div>

      <div>
        <label className="block text-sm font-medium text-gray-700 mb-1">
          Token Capacity (optional)
        </label>
        <input
          type="number"
          min="1"
          value={place.tokenCapacity || ''}
          onChange={(e) => handleCapacityChange(e.target.value)}
          placeholder="Unlimited"
          className="w-full px-3 py-2 border border-gray-300 rounded-md text-sm focus:ring-blue-500 focus:border-blue-500"
        />
      </div>

      {/* Type-specific parameters */}
      {place.params.type === 'entrypoint' && (
        <EntrypointConfig
          config={place.params.config}
          actors={actors}
          onUpdate={updateParams<{ type: 'entrypoint'; config: EntrypointParams }>}
        />
      )}

      {place.params.type === 'resource_pool' && (
        <ResourcePoolConfig
          config={place.params.config}
          actors={actors}
          onUpdate={updateParams<{ type: 'resource_pool'; config: ResourcePoolParams }>}
        />
      )}

      {place.params.type === 'action' && (
        <ActionConfig
          config={place.params.config}
          actions={actions}
          onUpdate={updateParams<{ type: 'action'; config: ActionParams }>}
        />
      )}

      {place.params.type === 'wait_with_timeout' && (
        <WaitTimeoutConfig
          config={place.params.config}
          onUpdate={updateParams<{ type: 'wait_with_timeout'; config: WaitWithTimeoutParams }>}
        />
      )}

      <div className="pt-4 border-t">
        <button
          onClick={handleDelete}
          className="w-full px-4 py-2 bg-red-600 text-white rounded-md hover:bg-red-700 text-sm"
        >
          Delete Place
        </button>
      </div>
    </div>
  );
}

// Entrypoint config
function EntrypointConfig({
  config,
  actors,
  onUpdate,
}: {
  config: EntrypointParams;
  actors: { id: string }[];
  onUpdate: (updates: Partial<EntrypointParams>) => void;
}) {
  return (
    <div className="space-y-3 p-3 bg-green-50 rounded-md">
      <h4 className="font-medium text-sm text-green-800">Entrypoint Config</h4>

      <div>
        <label className="block text-xs font-medium text-gray-600 mb-1">
          Entry Mechanism
        </label>
        <select
          value={config.entryMechanism}
          onChange={(e) =>
            onUpdate({ entryMechanism: e.target.value as 'http' | 'manual' | 'ros' })
          }
          className="w-full px-2 py-1 border border-gray-300 rounded text-sm"
        >
          <option value="http">HTTP</option>
          <option value="manual">Manual</option>
          <option value="ros">ROS</option>
        </select>
      </div>

      <div>
        <label className="block text-xs font-medium text-gray-600 mb-1">
          New Actors (comma-separated)
        </label>
        <input
          type="text"
          value={config.newActors.join(', ')}
          onChange={(e) =>
            onUpdate({
              newActors: e.target.value
                .split(',')
                .map((s) => s.trim())
                .filter(Boolean),
            })
          }
          placeholder="e.g., user::Vehicle"
          className="w-full px-2 py-1 border border-gray-300 rounded text-sm"
        />
        {actors.length > 0 && (
          <div className="text-xs text-gray-500 mt-1">
            Available: {actors.map((a) => a.id).join(', ')}
          </div>
        )}
      </div>
    </div>
  );
}

// Resource pool config
function ResourcePoolConfig({
  config,
  actors,
  onUpdate,
}: {
  config: ResourcePoolParams;
  actors: { id: string }[];
  onUpdate: (updates: Partial<ResourcePoolParams>) => void;
}) {
  return (
    <div className="space-y-3 p-3 bg-blue-50 rounded-md">
      <h4 className="font-medium text-sm text-blue-800">Resource Pool Config</h4>

      <div>
        <label className="block text-xs font-medium text-gray-600 mb-1">
          Resource Actor Type
        </label>
        <input
          type="text"
          value={config.resourceId}
          onChange={(e) => onUpdate({ resourceId: e.target.value })}
          placeholder="e.g., user::Charger"
          className="w-full px-2 py-1 border border-gray-300 rounded text-sm"
        />
        {actors.length > 0 && (
          <div className="text-xs text-gray-500 mt-1">
            Available: {actors.map((a) => a.id).join(', ')}
          </div>
        )}
      </div>
    </div>
  );
}

// Action config
function ActionConfig({
  config,
  actions,
  onUpdate,
}: {
  config: ActionParams;
  actions: { id: string }[];
  onUpdate: (updates: Partial<ActionParams>) => void;
}) {
  return (
    <div className="space-y-3 p-3 bg-orange-50 rounded-md">
      <h4 className="font-medium text-sm text-orange-800">Action Config</h4>

      <div>
        <label className="block text-xs font-medium text-gray-600 mb-1">
          Action ID
        </label>
        <input
          type="text"
          value={config.actionId}
          onChange={(e) => onUpdate({ actionId: e.target.value })}
          placeholder="e.g., user::move_to_location"
          className="w-full px-2 py-1 border border-gray-300 rounded text-sm"
        />
        {actions.length > 0 && (
          <div className="text-xs text-gray-500 mt-1">
            Available: {actions.map((a) => a.id).join(', ')}
          </div>
        )}
      </div>

      <div className="grid grid-cols-2 gap-2">
        <div>
          <label className="block text-xs font-medium text-gray-600 mb-1">
            Retries
          </label>
          <input
            type="number"
            min="0"
            value={config.retries || 0}
            onChange={(e) => onUpdate({ retries: parseInt(e.target.value, 10) })}
            className="w-full px-2 py-1 border border-gray-300 rounded text-sm"
          />
        </div>
        <div>
          <label className="block text-xs font-medium text-gray-600 mb-1">
            Timeout (s)
          </label>
          <input
            type="number"
            min="1"
            value={config.timeoutPerTryS || 60}
            onChange={(e) =>
              onUpdate({ timeoutPerTryS: parseInt(e.target.value, 10) })
            }
            className="w-full px-2 py-1 border border-gray-300 rounded text-sm"
          />
        </div>
      </div>

      <div className="space-y-2">
        <label className="flex items-center space-x-2">
          <input
            type="checkbox"
            checked={config.failureAsError || false}
            onChange={(e) => onUpdate({ failureAsError: e.target.checked })}
            className="rounded"
          />
          <span className="text-xs text-gray-700">Failure as Error</span>
        </label>
        <label className="flex items-center space-x-2">
          <input
            type="checkbox"
            checked={config.errorToGlobalHandler || false}
            onChange={(e) => onUpdate({ errorToGlobalHandler: e.target.checked })}
            className="rounded"
          />
          <span className="text-xs text-gray-700">Error to Global Handler</span>
        </label>
      </div>
    </div>
  );
}

// Wait with timeout config
function WaitTimeoutConfig({
  config,
  onUpdate,
}: {
  config: WaitWithTimeoutParams;
  onUpdate: (updates: Partial<WaitWithTimeoutParams>) => void;
}) {
  return (
    <div className="space-y-3 p-3 bg-yellow-50 rounded-md">
      <h4 className="font-medium text-sm text-yellow-800">Wait Config</h4>

      <div>
        <label className="block text-xs font-medium text-gray-600 mb-1">
          Timeout (minutes)
        </label>
        <input
          type="number"
          min="1"
          value={config.timeoutMin}
          onChange={(e) =>
            onUpdate({ timeoutMin: parseInt(e.target.value, 10) })
          }
          className="w-full px-2 py-1 border border-gray-300 rounded text-sm"
        />
      </div>

      <div>
        <label className="block text-xs font-medium text-gray-600 mb-1">
          On Timeout (error type)
        </label>
        <input
          type="text"
          value={config.onTimeout}
          onChange={(e) => onUpdate({ onTimeout: e.target.value })}
          placeholder="e.g., error::timeout"
          className="w-full px-2 py-1 border border-gray-300 rounded text-sm"
        />
      </div>
    </div>
  );
}
