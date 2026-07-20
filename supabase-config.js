const SUPABASE_URL = 'https://cwnvmflkffmxklzhlukk.supabase.co';
const SUPABASE_ANON_KEY = 'sb_publishable_afIfkH02YZBiCamLSuRsKQ_OPfM2Czi';

window.supabaseClient = supabase.createClient(SUPABASE_URL, SUPABASE_ANON_KEY);
