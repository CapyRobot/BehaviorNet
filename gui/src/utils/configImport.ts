import type {
  BNetConfig,
  Place,
  Transition,
  ActorDefinition,
  ActionDefinition,
  PlaceType,
  PlaceParams,
} from '../store/types';

export function importFromJson(json: string): BNetConfig {
  const data = JSON.parse(json);

  // Extract GUI metadata for positions
  const guiMetadata = data._gui_metadata || {};
  const placePositions = guiMetadata.places || {};
  const transitionPositions = guiMetadata.transitions || {};

  // Import actors
  const actors: ActorDefinition[] = (data.actors || []).map(
    (a: Record<string, unknown>): ActorDefinition => {
      const actor: ActorDefinition = {
        id: a.id as string,
        requiredInitParams: (a.required_init_params || {}) as Record<
          string,
          { type: string }
        >,
        optionalInitParams: a.optional_init_params as
          | Record<string, { type: string }>
          | undefined,
      };
      if (a.type === 'http_actor' && a.services) {
        actor.httpConfig = {
          services: a.services as NonNullable<ActorDefinition['httpConfig']>['services'],
        };
      }
      return actor;
    }
  );

  // Import actions
  const actions: ActionDefinition[] = (data.actions || []).map(
    (a: Record<string, unknown>): ActionDefinition => {
      const action: ActionDefinition = {
        id: a.id as string,
        requiredActors: (a.required_actors || []) as string[],
      };
      if (a.type === 'http_actor_service' && a.params) {
        const params = a.params as Record<string, unknown>;
        action.httpConfig = {
          actorId: params.actor_id as string,
          serviceId: params.service_id as string,
          serviceParams: params.service_params as Record<string, unknown> | undefined,
        };
      }
      return action;
    }
  );

  // Import places
  const places: Place[] = (data.places || []).map(
    (p: Record<string, unknown>, index: number) => {
      const id = p.id as string;
      const type = p.type as PlaceType;
      const position = placePositions[id]?.position || {
        x: 100 + (index % 5) * 150,
        y: 100 + Math.floor(index / 5) * 150,
      };

      return {
        id,
        label: id,
        type,
        params: importPlaceParams(type, p.params as Record<string, unknown>),
        position,
        tokenCapacity: p.token_capacity as number | undefined,
        requiredActors: p.required_actors as string[] | undefined,
      };
    }
  );

  // Import transitions
  const transitions: Transition[] = (data.transitions || []).map(
    (t: Record<string, unknown>, index: number) => {
      // Generate a transition ID if not present
      const id = `transition_${index + 1}`;
      const position = transitionPositions[id]?.position || {
        x: 250 + (index % 5) * 150,
        y: 100 + Math.floor(index / 5) * 150,
      };

      const from = t.from as string[];
      const toRaw = t.to as Array<string | { to: string; token_filter?: string }>;

      const to = toRaw.map((output) => {
        if (typeof output === 'string') {
          return { to: output };
        }
        return {
          to: output.to,
          tokenFilter: output.token_filter,
        };
      });

      return {
        id,
        label: 'T',
        from,
        to,
        priority: (t.priority as number) || 1,
        position,
      };
    }
  );

  return {
    actors,
    actions,
    places,
    transitions,
    // TODO: Import global error handler if present
  };
}

function importPlaceParams(
  type: PlaceType,
  params: Record<string, unknown> | undefined
): PlaceParams {
  switch (type) {
    case 'entrypoint':
      return {
        type: 'entrypoint',
        config: {
          entryMechanism: (params?.entry_mechanism as 'http' | 'manual' | 'ros') || 'http',
          newActors: (params?.new_actors as string[]) || [],
        },
      };
    case 'resource_pool':
      return {
        type: 'resource_pool',
        config: {
          resourceId: (params?.resource_id as string) || '',
        },
      };
    case 'action':
      return {
        type: 'action',
        config: {
          actionId: (params?.action_id as string) || '',
          retries: params?.retries as number | undefined,
          timeoutPerTryS: params?.timeout_per_try_s as number | undefined,
          timeoutPerTryMin: params?.timeout_per_try_min as number | undefined,
          timeoutAsError: params?.timeout_as_error as boolean | undefined,
          failureAsError: params?.failure_as_error as boolean | undefined,
          errorToGlobalHandler: params?.error_to_global_handler as boolean | undefined,
        },
      };
    case 'wait_with_timeout':
      return {
        type: 'wait_with_timeout',
        config: {
          timeoutMin: (params?.timeout_min as number) || 5,
          onTimeout: (params?.on_timeout as string) || 'error::timeout',
        },
      };
    case 'exit_logger':
      return { type: 'exit_logger', config: {} };
    case 'plain':
    default:
      return { type: 'plain', config: {} };
  }
}
