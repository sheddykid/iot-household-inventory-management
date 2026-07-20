const supabase = window.supabaseClient;

<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<script src="https://cdn.jsdelivr.net/npm/@supabase/supabase-js@2"></script>
<script src="supabase-config.js"></script>
<script src="auth.js"></script>
<script src="dashboard.js"></script>

(async () => {
    const { data } = await supabase.auth.getSession();
    if (!data.session) {
        location.href = "login.html";
        return;
    }
    document.getElementById("loggedUser").textContent =
        data.session.user.email;
})();
document
.getElementById("logoutBtn")
.addEventListener("click", async () => {
    await supabase.auth.signOut();
    location.href = "login.html";
});
