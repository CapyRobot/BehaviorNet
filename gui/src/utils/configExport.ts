import type { BNetConfig, Place, Transition } from '../store/types';

// Convert internal format to export format matching DESIGN.md spec
export function exportToJson(config: BNetConfig): string {
  const exportConfig = {
    actors: config.actors.map((actor) => ({
      id: actor.id,
      required_init_params: actor.requiredInitParams,
      optional_init_params: actor.optionalInitParams,
      ...(actor.httpConfig && {
        type: 'http_actor',
        services: actor.httpConfig.services,
      }),
    })),
    actions: config.actions.map((action) => ({
      id: action.id,
      required_actors: action.requiredActors,
      ...(action.httpConfig && {
        type: 'http_actor_service',
        params: {
          actor_id: action.httpConfig.actorId,
          service_id: action.httpConfig.serviceId,
          service_params: action.httpConfig.serviceParams,
        },
      }),
    })),
    places: config.places.map((place) => exportPlace(place)),
    transitions: config.transitions.map((transition) => exportTransition(transition)),
    ...(config.globalErrorHandler && {
      global_error_handler: {
        places: config.globalErrorHandler.places.map((p) => exportPlace(p)),
        mapping: config.globalErrorHandler.mapping.map((m) => ({
          error_id_filter: m.errorIdFilter,
          actor_filter: m.actorFilter,
          destination: m.destination,
        })),
      },
    }),
    // GUI metadata (positions)
    _gui_metadata: {
      places: Object.fromEntries(
        config.places.map((p) => [p.id, { position: p.position }])
      ),
      transitions: Object.fromEntries(
        config.transitions.map((t) => [t.id, { position: t.position }])
      ),
    },
  };

  return JSON.stringify(exportConfig, null, 2);
}

function exportPlace(place: Place): Record<string, unknown> {
  const base: Record<string, unknown> = {
    id: place.id,
    type: place.type,
  };

  if (place.tokenCapacity) {
    base.token_capacity = place.tokenCapacity;
  }

  if (place.requiredActors && place.requiredActors.length > 0) {
    base.required_actors = place.requiredActors;
  }

  // Type-specific params
  switch (place.params.type) {
    case 'entrypoint':
      base.params = {
        entry_mechanism: place.params.config.entryMechanism,
        new_actors: place.params.config.newActors,
      };
      break;
    case 'resource_pool':
      base.params = {
        resource_id: place.params.config.resourceId,
      };
      break;
    case 'action':
      base.params = {
        action_id: place.params.config.actionId,
        ...(place.params.config.retries !== undefined && {
          retries: place.params.config.retries,
        }),
        ...(place.params.config.timeoutPerTryS !== undefined && {
          timeout_per_try_s: place.params.config.timeoutPerTryS,
        }),
        ...(place.params.config.timeoutPerTryMin !== undefined && {
          timeout_per_try_min: place.params.config.timeoutPerTryMin,
        }),
        ...(place.params.config.timeoutAsError !== undefined && {
          timeout_as_error: place.params.config.timeoutAsError,
        }),
        ...(place.params.config.failureAsError !== undefined && {
          failure_as_error: place.params.config.failureAsError,
        }),
        ...(place.params.config.errorToGlobalHandler !== undefined && {
          error_to_global_handler: place.params.config.errorToGlobalHandler,
        }),
      };
      break;
    case 'wait_with_timeout':
      base.params = {
        timeout_min: place.params.config.timeoutMin,
        on_timeout: place.params.config.onTimeout,
      };
      break;
    case 'exit_logger':
    case 'plain':
      // No params needed
      break;
  }

  return base;
}

function exportTransition(transition: Transition): Record<string, unknown> {
  const result: Record<string, unknown> = {
    from: transition.from,
    to: transition.to.map((output) => {
      if (output.tokenFilter) {
        return { to: output.to, token_filter: output.tokenFilter };
      }
      return output.to;
    }),
  };

  if (transition.priority && transition.priority !== 1) {
    result.priority = transition.priority;
  }

  return result;
}

export function downloadJson(json: string, filename: string): void {
  const blob = new Blob([json], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = filename;
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  URL.revokeObjectURL(url);
}
