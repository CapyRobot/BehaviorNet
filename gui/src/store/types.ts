// Place in the Petri net - uses generic params based on type
export interface Place {
  id: string;
  label: string;
  type: string; // Place type from places.json
  params: Record<string, unknown>; // Type-specific parameters
  position: { x: number; y: number };
  tokenCapacity?: number;
  requiredActors?: string[];
}

// Transition output with optional token filter
export interface TransitionOutput {
  to: string; // Place ID
  tokenFilter?: string; // Actor type to filter for this output
}

// Transition in the Petri net
export interface Transition {
  id: string;
  label: string;
  from: string[]; // Place IDs (including subplaces like "action::success")
  to: TransitionOutput[];
  priority?: number;
  position: { x: number; y: number };
}

// Actor definition - describes an actor type that can be used in tokens
export interface ActorDefinition {
  id: string; // e.g., "user::Vehicle"
  requiredInitParams: Record<string, { type: string }>;
  optionalInitParams?: Record<string, { type: string }>;
  // For HTTP actors
  httpConfig?: {
    services: HttpService[];
  };
}

export interface HttpService {
  id: string;
  description: string;
  method: 'GET' | 'POST' | 'PUT' | 'DELETE';
  url: string;
  body?: Record<string, unknown>;
  responsePath?: string;
}

// Action definition - describes an action that can be invoked on actors
export interface ActionDefinition {
  id: string; // e.g., "user::move_to_location"
  requiredActors: string[]; // Actor IDs required for this action
  // For HTTP-based actions
  httpConfig?: {
    actorId: string;
    serviceId: string;
    serviceParams?: Record<string, unknown>;
  };
}

// Error handler mapping
export interface ErrorMapping {
  errorIdFilter: string; // Error type filter, "*" for all
  actorFilter?: string; // Actor type filter
  destination: string; // Place ID to route to
}

// Global error handler configuration
export interface GlobalErrorHandler {
  places: Place[];
  mapping: ErrorMapping[];
}

// Complete net configuration (for export/import)
export interface BNetConfig {
  actors: ActorDefinition[];
  actions: ActionDefinition[];
  places: Place[];
  transitions: Transition[];
  globalErrorHandler?: GlobalErrorHandler;
}
