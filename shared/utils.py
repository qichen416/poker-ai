def parse_action_string(s: str):
    parts = s.split()
    action = parts[0].lower()
    amount = int(parts[1]) if len(parts) > 1 else 0
    return action, amount

def format_action_string(action: str, amount: int = 0) -> str:
    if action in ['raise', 'bet']:
        return "%s %d" % (action, amount)
    return action
