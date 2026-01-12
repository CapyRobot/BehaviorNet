import { useState } from 'react';
import { useNetStore } from '../../store/netStore';
import type { ActorDefinition, ActionDefinition } from '../../store/types';

interface Props {
  onClose: () => void;
}

export default function ActorRegistry({ onClose }: Props) {
  const [activeTab, setActiveTab] = useState<'actors' | 'actions'>('actors');

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
      <div className="bg-white rounded-lg shadow-xl w-[600px] max-h-[80vh] flex flex-col">
        <div className="flex items-center justify-between p-4 border-b">
          <h2 className="text-lg font-bold">Actor & Action Registry</h2>
          <button
            onClick={onClose}
            className="text-gray-500 hover:text-gray-700 text-xl"
          >
            x
          </button>
        </div>

        <div className="flex border-b">
          <button
            onClick={() => setActiveTab('actors')}
            className={`flex-1 py-2 text-sm font-medium ${
              activeTab === 'actors'
                ? 'text-blue-600 border-b-2 border-blue-600'
                : 'text-gray-500 hover:text-gray-700'
            }`}
          >
            Actors
          </button>
          <button
            onClick={() => setActiveTab('actions')}
            className={`flex-1 py-2 text-sm font-medium ${
              activeTab === 'actions'
                ? 'text-blue-600 border-b-2 border-blue-600'
                : 'text-gray-500 hover:text-gray-700'
            }`}
          >
            Actions
          </button>
        </div>

        <div className="flex-1 overflow-y-auto p-4">
          {activeTab === 'actors' ? <ActorsPanel /> : <ActionsPanel />}
        </div>
      </div>
    </div>
  );
}

function ActorsPanel() {
  const actors = useNetStore((s) => s.actors);
  const addActor = useNetStore((s) => s.addActor);
  const updateActor = useNetStore((s) => s.updateActor);
  const deleteActor = useNetStore((s) => s.deleteActor);

  const [newActorId, setNewActorId] = useState('');

  const handleAddActor = () => {
    if (!newActorId.trim()) return;
    if (actors.some((a) => a.id === newActorId)) {
      alert('Actor ID already exists');
      return;
    }
    addActor({
      id: newActorId.trim(),
      requiredInitParams: {},
    });
    setNewActorId('');
  };

  return (
    <div className="space-y-4">
      <div className="flex space-x-2">
        <input
          type="text"
          value={newActorId}
          onChange={(e) => setNewActorId(e.target.value)}
          placeholder="e.g., user::Vehicle"
          className="flex-1 px-3 py-2 border border-gray-300 rounded-md text-sm"
          onKeyDown={(e) => e.key === 'Enter' && handleAddActor()}
        />
        <button
          onClick={handleAddActor}
          className="px-4 py-2 bg-blue-600 text-white rounded-md hover:bg-blue-500 text-sm"
        >
          Add Actor
        </button>
      </div>

      {actors.length === 0 ? (
        <p className="text-sm text-gray-500 text-center py-4">
          No actors defined. Add actors to use in your net.
        </p>
      ) : (
        <div className="space-y-3">
          {actors.map((actor) => (
            <ActorCard
              key={actor.id}
              actor={actor}
              onUpdate={(updates) => updateActor(actor.id, updates)}
              onDelete={() => deleteActor(actor.id)}
            />
          ))}
        </div>
      )}
    </div>
  );
}

function ActorCard({
  actor,
  onUpdate,
  onDelete,
}: {
  actor: ActorDefinition;
  onUpdate: (updates: Partial<ActorDefinition>) => void;
  onDelete: () => void;
}) {
  const [expanded, setExpanded] = useState(false);
  const [newParamKey, setNewParamKey] = useState('');
  const [newParamType, setNewParamType] = useState('str');

  const handleAddParam = () => {
    if (!newParamKey.trim()) return;
    onUpdate({
      requiredInitParams: {
        ...actor.requiredInitParams,
        [newParamKey]: { type: newParamType },
      },
    });
    setNewParamKey('');
  };

  const handleRemoveParam = (key: string) => {
    const { [key]: removed, ...rest } = actor.requiredInitParams;
    onUpdate({ requiredInitParams: rest });
  };

  return (
    <div className="border border-gray-200 rounded-lg p-3">
      <div className="flex items-center justify-between">
        <button
          onClick={() => setExpanded(!expanded)}
          className="flex items-center space-x-2 text-left"
        >
          <span className="text-sm">{expanded ? '[-]' : '[+]'}</span>
          <span className="font-medium">{actor.id}</span>
        </button>
        <button
          onClick={onDelete}
          className="text-red-500 hover:text-red-700 text-sm"
        >
          Delete
        </button>
      </div>

      {expanded && (
        <div className="mt-3 pl-4 space-y-2">
          <div className="text-xs text-gray-500 font-medium">
            Required Init Params:
          </div>
          {Object.entries(actor.requiredInitParams).length === 0 ? (
            <p className="text-xs text-gray-400">No parameters defined</p>
          ) : (
            <div className="space-y-1">
              {Object.entries(actor.requiredInitParams).map(([key, value]) => (
                <div
                  key={key}
                  className="flex items-center justify-between bg-gray-50 px-2 py-1 rounded text-xs"
                >
                  <span>
                    {key}: <span className="text-blue-600">{value.type}</span>
                  </span>
                  <button
                    onClick={() => handleRemoveParam(key)}
                    className="text-red-500 hover:text-red-700"
                  >
                    x
                  </button>
                </div>
              ))}
            </div>
          )}
          <div className="flex space-x-2 mt-2">
            <input
              type="text"
              value={newParamKey}
              onChange={(e) => setNewParamKey(e.target.value)}
              placeholder="Param name"
              className="flex-1 px-2 py-1 border border-gray-300 rounded text-xs"
            />
            <select
              value={newParamType}
              onChange={(e) => setNewParamType(e.target.value)}
              className="px-2 py-1 border border-gray-300 rounded text-xs"
            >
              <option value="str">str</option>
              <option value="int">int</option>
              <option value="float">float</option>
              <option value="bool">bool</option>
            </select>
            <button
              onClick={handleAddParam}
              className="px-2 py-1 bg-gray-200 hover:bg-gray-300 rounded text-xs"
            >
              Add
            </button>
          </div>
        </div>
      )}
    </div>
  );
}

function ActionsPanel() {
  const actions = useNetStore((s) => s.actions);
  const actors = useNetStore((s) => s.actors);
  const addAction = useNetStore((s) => s.addAction);
  const updateAction = useNetStore((s) => s.updateAction);
  const deleteAction = useNetStore((s) => s.deleteAction);

  const [newActionId, setNewActionId] = useState('');

  const handleAddAction = () => {
    if (!newActionId.trim()) return;
    if (actions.some((a) => a.id === newActionId)) {
      alert('Action ID already exists');
      return;
    }
    addAction({
      id: newActionId.trim(),
      requiredActors: [],
    });
    setNewActionId('');
  };

  return (
    <div className="space-y-4">
      <div className="flex space-x-2">
        <input
          type="text"
          value={newActionId}
          onChange={(e) => setNewActionId(e.target.value)}
          placeholder="e.g., user::move_to_location"
          className="flex-1 px-3 py-2 border border-gray-300 rounded-md text-sm"
          onKeyDown={(e) => e.key === 'Enter' && handleAddAction()}
        />
        <button
          onClick={handleAddAction}
          className="px-4 py-2 bg-blue-600 text-white rounded-md hover:bg-blue-500 text-sm"
        >
          Add Action
        </button>
      </div>

      {actions.length === 0 ? (
        <p className="text-sm text-gray-500 text-center py-4">
          No actions defined. Add actions to use in action places.
        </p>
      ) : (
        <div className="space-y-3">
          {actions.map((action) => (
            <ActionCard
              key={action.id}
              action={action}
              actors={actors}
              onUpdate={(updates) => updateAction(action.id, updates)}
              onDelete={() => deleteAction(action.id)}
            />
          ))}
        </div>
      )}
    </div>
  );
}

function ActionCard({
  action,
  actors,
  onUpdate,
  onDelete,
}: {
  action: ActionDefinition;
  actors: ActorDefinition[];
  onUpdate: (updates: Partial<ActionDefinition>) => void;
  onDelete: () => void;
}) {
  const [expanded, setExpanded] = useState(false);

  const handleToggleActor = (actorId: string) => {
    const current = action.requiredActors;
    if (current.includes(actorId)) {
      onUpdate({ requiredActors: current.filter((a) => a !== actorId) });
    } else {
      onUpdate({ requiredActors: [...current, actorId] });
    }
  };

  return (
    <div className="border border-gray-200 rounded-lg p-3">
      <div className="flex items-center justify-between">
        <button
          onClick={() => setExpanded(!expanded)}
          className="flex items-center space-x-2 text-left"
        >
          <span className="text-sm">{expanded ? '[-]' : '[+]'}</span>
          <span className="font-medium">{action.id}</span>
        </button>
        <button
          onClick={onDelete}
          className="text-red-500 hover:text-red-700 text-sm"
        >
          Delete
        </button>
      </div>

      {expanded && (
        <div className="mt-3 pl-4 space-y-2">
          <div className="text-xs text-gray-500 font-medium">
            Required Actors:
          </div>
          {actors.length === 0 ? (
            <p className="text-xs text-gray-400">
              Define actors first to select required actors
            </p>
          ) : (
            <div className="space-y-1">
              {actors.map((actor) => (
                <label
                  key={actor.id}
                  className="flex items-center space-x-2 text-xs"
                >
                  <input
                    type="checkbox"
                    checked={action.requiredActors.includes(actor.id)}
                    onChange={() => handleToggleActor(actor.id)}
                    className="rounded"
                  />
                  <span>{actor.id}</span>
                </label>
              ))}
            </div>
          )}
        </div>
      )}
    </div>
  );
}
