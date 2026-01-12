import { useState, useEffect } from 'react';
import { ReactFlowProvider } from '@xyflow/react';
import NetCanvas from './components/Canvas/NetCanvas';
import Sidebar from './components/Sidebar/Sidebar';
import Inspector from './components/Inspector/Inspector';
import Toolbar from './components/Toolbar/Toolbar';
import ActorRegistry from './components/Sidebar/ActorRegistry';
import { loadPlaceConfig, type PlaceConfigSchema } from './store/placeConfig';

export default function App() {
  const [showRegistry, setShowRegistry] = useState(false);
  const [placeConfig, setPlaceConfig] = useState<PlaceConfigSchema | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    loadPlaceConfig()
      .then(setPlaceConfig)
      .finally(() => setLoading(false));
  }, []);

  if (loading) {
    return (
      <div className="h-screen flex items-center justify-center bg-gray-100">
        <div className="text-gray-600">Loading configuration...</div>
      </div>
    );
  }

  if (!placeConfig) {
    return (
      <div className="h-screen flex items-center justify-center bg-gray-100">
        <div className="text-red-600">Failed to load configuration</div>
      </div>
    );
  }

  return (
    <ReactFlowProvider>
      <div className="h-screen flex flex-col">
        <Toolbar onShowRegistry={() => setShowRegistry(true)} />
        <div className="flex-1 flex overflow-hidden">
          <Sidebar placeConfig={placeConfig} />
          <NetCanvas placeConfig={placeConfig} />
          <Inspector placeConfig={placeConfig} />
        </div>
        {showRegistry && (
          <ActorRegistry onClose={() => setShowRegistry(false)} />
        )}
      </div>
    </ReactFlowProvider>
  );
}
