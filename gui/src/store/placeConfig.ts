// Place configuration loaded from places.json

export interface ParamDefinition {
  type: 'string' | 'integer' | 'boolean' | 'enum' | 'array' | 'actorRef' | 'actionRef';
  required?: boolean;
  default?: unknown;
  description?: string;
  min?: number;
  max?: number;
  options?: string[];
  items?: { type: string };
}

export interface PlaceTypeDefinition {
  label: string;
  description: string;
  color: string;
  icon: string;
  hasSubplaces?: boolean;
  subplaces?: string[];
  params: Record<string, ParamDefinition>;
}

export interface PlaceConfigSchema {
  commonParams: Record<string, ParamDefinition>;
  placeTypes: Record<string, PlaceTypeDefinition>;
  transitionParams: Record<string, ParamDefinition>;
  arcParams: Record<string, ParamDefinition>;
}

let cachedConfig: PlaceConfigSchema | null = null;

export async function loadPlaceConfig(): Promise<PlaceConfigSchema> {
  if (cachedConfig) return cachedConfig;

  try {
    const response = await fetch('/places.json');
    if (!response.ok) {
      throw new Error(`Failed to load places.json: ${response.status}`);
    }
    cachedConfig = await response.json();
    return cachedConfig!;
  } catch (error) {
    console.error('Failed to load place config, using defaults:', error);
    return getDefaultConfig();
  }
}

export function getPlaceConfig(): PlaceConfigSchema | null {
  return cachedConfig;
}

// Default config in case places.json fails to load
function getDefaultConfig(): PlaceConfigSchema {
  return {
    commonParams: {
      id: { type: 'string', required: true, description: 'Unique identifier' },
      label: { type: 'string', required: false, description: 'Display label' },
      tokenCapacity: { type: 'integer', required: false, min: 1, description: 'Max tokens' },
    },
    placeTypes: {
      entrypoint: {
        label: 'Entrypoint',
        description: 'Entry point for new tokens',
        color: 'green',
        icon: '>',
        params: {
          entryMechanism: { type: 'enum', options: ['http', 'manual', 'ros'], default: 'http', required: true },
          newActors: { type: 'array', items: { type: 'actorRef' }, default: [] },
        },
      },
      resource_pool: {
        label: 'Resource Pool',
        description: 'Pool of available resources',
        color: 'blue',
        icon: '#',
        params: {
          resourceId: { type: 'actorRef', required: true },
        },
      },
      action: {
        label: 'Action',
        description: 'Executes an action on the token',
        color: 'orange',
        icon: 'A',
        hasSubplaces: true,
        subplaces: ['success', 'failure', 'error'],
        params: {
          actionId: { type: 'actionRef', required: true },
          retries: { type: 'integer', min: 0, default: 0 },
          timeoutPerTryS: { type: 'integer', min: 1 },
          failureAsError: { type: 'boolean', default: false },
          errorToGlobalHandler: { type: 'boolean', default: true },
        },
      },
      wait_with_timeout: {
        label: 'Wait with Timeout',
        description: 'Waits for a condition with timeout',
        color: 'yellow',
        icon: 'W',
        params: {
          timeoutMin: { type: 'integer', min: 1, default: 5, required: true },
          onTimeout: { type: 'string', default: 'error::timeout', required: true },
        },
      },
      exit_logger: {
        label: 'Exit Logger',
        description: 'Logs token and destroys it',
        color: 'red',
        icon: 'X',
        params: {},
      },
      plain: {
        label: 'Plain',
        description: 'Simple place for flow control',
        color: 'gray',
        icon: 'P',
        params: {},
      },
    },
    transitionParams: {
      priority: { type: 'integer', min: 1, default: 1 },
    },
    arcParams: {
      tokenFilter: { type: 'actorRef' },
    },
  };
}

// Helper to get default values for a place type's params
export function getDefaultParamsForType(
  config: PlaceConfigSchema,
  placeType: string
): Record<string, unknown> {
  const typeDef = config.placeTypes[placeType];
  if (!typeDef) return {};

  const params: Record<string, unknown> = {};
  for (const [key, paramDef] of Object.entries(typeDef.params)) {
    if (paramDef.default !== undefined) {
      params[key] = paramDef.default;
    }
  }
  return params;
}
