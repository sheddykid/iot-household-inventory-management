// ===============================
// auth.js
// Household Inventory Management
// ===============================

// Create Supabase Client
const supabase = window.supabaseClient;

// Elements
const loginForm = document.getElementById("loginForm");
const registerForm = document.getElementById("registerForm");
const forgotBtn = document.getElementById("forgotPassword");
const msgBox = document.getElementById("msg");

// -------------------------------
// Show Messages
// -------------------------------

function showMessage(message, type = "success") {

    msgBox.className = `message ${type}`;

    msgBox.innerHTML = message;

    msgBox.style.display = "block";

    setTimeout(() => {

        msgBox.style.display = "none";

    }, 5000);

}


// -------------------------------
// Already Logged In?
// -------------------------------

(async () => {

    const { data } = await supabase.auth.getSession();

    if (data.session) {

        window.location.href = "index.html";

    }

})();


// ===============================
// LOGIN
// ===============================

loginForm.addEventListener("submit", async (e) => {

    e.preventDefault();

    const email =
        document.getElementById("loginEmail").value.trim();

    const password =
        document.getElementById("loginPassword").value;

    const {

        error

    } = await supabase.auth.signInWithPassword({

        email,

        password

    });

    if (error) {

        showMessage(error.message, "error");

        return;

    }

    showMessage("Login successful.");

    setTimeout(() => {

        window.location.href = "index.html";

    }, 1000);

});


// ===============================
// REGISTER
// ===============================

registerForm.addEventListener("submit", async (e) => {

    e.preventDefault();

    const fullName =
        document.getElementById("registerName").value.trim();

    const email =
        document.getElementById("registerEmail").value.trim();

    const password =
        document.getElementById("registerPassword").value;

    const confirm =
        document.getElementById("confirmPassword").value;

    if (password !== confirm) {

        showMessage("Passwords do not match.", "error");

        return;

    }

    const {

        data,

        error

    } = await supabase.auth.signUp({

        email,

        password,

        options: {

            data: {

                full_name: fullName

            }

        }

    });

    if (error) {

        showMessage(error.message, "error");

        return;

    }

    // Save profile

    if (data.user) {

        await supabase

            .from("profiles")

            .upsert({

                id: data.user.id,

                full_name: fullName,

                email: email

            });

    }

    showMessage(

        "Registration successful. Check your email to verify your account."

    );

    registerForm.reset();

});


// ===============================
// FORGOT PASSWORD
// ===============================

forgotBtn.addEventListener("click", async (e) => {

    e.preventDefault();

    const email = prompt("Enter your email address");

    if (!email) return;

    const {

        error

    } = await supabase.auth.resetPasswordForEmail(email, {

        redirectTo:
            window.location.origin + "/reset-password.html"

    });

    if (error) {

        showMessage(error.message, "error");

        return;

    }

    showMessage(

        "Password reset email sent."

    );

});


// ===============================
// Listen For Auth Changes
// ===============================

supabase.auth.onAuthStateChange((event, session) => {

    console.log(event);

    if (event === "SIGNED_OUT") {

        window.location.href = "login.html";

    }

});
